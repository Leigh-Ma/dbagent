#include "../pub/pub.h"
#include "../table/table_pub.h"

TCOM *table_com_init_by_ti(const TI *ti) {
    TCOM *tc = (TCOM*)0;

    _CHECK_PARAMS_RET(ti != (TI*)0, tc);

    tc = (TCOM*)malloc(sizeof(TCOM));
    if(tc == (TCOM*)0) {
        logger("Allocate memory for <%s>'s TCOM error.\n", ti->name_ex);
        return tc;
    }

    memset(tc, 0x00, sizeof(TCOM));
    tc->table_no = ti->tno;
    tc->ti       = ti;

    return tc;
}

TCOM *table_com_init_by_tname(const char *table_name) {
    TCOM *tc = (TCOM*)0;
    TI *ti;

    _CHECK_PARAMS_RET(table_name != (char*)0 && table_name[0] != '\0', tc);

    ti = query_get_table_info_by_name(table_name);
    tc = table_com_init_by_ti(ti);

    if(tc == (TCOM*)0) {
        logger("Initialization for <%s>'s TCOM error.\n", table_name);
    }

    return tc;
}

TCOM *table_com_init_by_tno(const UINT32 table_no) {
    TCOM *tc = (TCOM*)0;
    TI *ti;

    ti = query_get_table_info_by_tno(table_no);
    tc = table_com_init_by_ti(ti);

    if(tc == (TCOM*)0) {
        logger("Initialization for table no.<%d>'s TCOM error.\n", table_no);
    }

    return tc;
}

RCOM* table_com_to_row_com(TCOM *tc) {
    if(tc == (TCOM*)0 ) {
       return tc;
    }
    tc->record_num = 1;
    _set_tc_flag_rc(tc);
    return tc;
}

/* use after release data, or right after initialization, tc is group leader row */
TCOM *table_com_load_data(TCOM* tc, const char *condition) {
    INT32 status = PR_OK, len;
    TCOM  *more_tc, *last_tc;
    condition = condition ? condition : "";

    if(tc == (TCOM*)0) {
        return tc;
    }

    _set_tc_flag_is_rc(tc);             /* leading record */
    len = strlen(condition) + 1;

    tc->condition = (char*)malloc(len);
    if(tc->condition == (char*)0){
        _set_tc_flag_condition_set_failed(tc);
        return tc;
    }
    snprintf(tc->condition, len, "%s", condition);

    status = find_rows_with_cond_with_ti(tc->ti, condition, &tc->should_free, &tc->record_num);
    if(status != PR_OK) {
        _set_tc_flag_query_failed(tc);
        return tc;
    }

    if(tc->record_num <= 0 || tc->should_free==(char*)0) {
        _set_tc_flag_query_no_data(tc);
        return tc;
    }

    _set_tc_flag_data_loaded(tc);
    tc->record = tc->should_free;

    last_tc = tc;
    for(status = 1; status < tc->record_num; status++) {
        more_tc = table_com_init_by_ti(tc->ti);
        if(more_tc == (TCOM*)0) {
            _set_tc_flag_tc_list_error(tc);
        } else {
            more_tc->record_num = tc->record_num;
            more_tc->record = tc->record + tc->ti->row_size;
            more_tc->next = last_tc;
            _set_tc_flag_data_loaded(tc);
            last_tc = more_tc;
        }
    }
    last_tc->next = tc;

    return tc;
}

/* reload current row only */
TCOM *row_com_reload_data(TCOM* tc) {
    INT32 status = PR_OK, record_num = 1;
    char  condition[32] = {0};
    TCOM  *more_tc, *last_tc;
    void  *reload;

    if(tc == (TCOM*)0 || tc->record == (char*)0) {
        return tc;
    }

    snprintf(condition, 32, "where id = %d", *(UINT32)tc->record);
    status = find_rows_with_cond_with_ti(tc->ti, condition, &reload, &record_num);
    if(status != PR_OK || record_num > 1) {
        _set_tc_flag_reload_failed(tc);
        return tc;
    }

    if(record_num <= 0 || reload ==(char*)0) {
        _set_tc_flag_query_no_data(tc);
        tc->record = (char*)0;
        return tc;
    }

    memcpy(tc->record, reload, tc->ti->row_size);
    _set_tc_flag_data_loaded(tc);
    free(reload);

    return tc;
}

/* reload all row in the table_com list according to condition
 * @condition: to use old condition, use null as condition
 *             to use new condition, use none null (blank is valid)
 */
TCOM *table_com_reload_data(TCOM* tc, const char *condition) {
    INT32 status = PR_OK, len, len_old;
    TCOM  *more_tc, *last_tc;
    void  *reload, *cond;


    if(tc == (TCOM*)0 || tc->record == (char*)0 || tc->record_num <= 0) {
        return tc;
    }

    if(!tc_flag_is_leader(tc)) {
        _set_tc_warning_not_leader(tc);
        return tc;
    }
    if(tc->should_free) {
        free(tc->should_free);
        tc->should_free = 0;
        tc->record_num  = 0; /* TODO, what if record_num is used as a limit at initialization */
    }
    cond = condition ? condition : tc->condition;
    if(condition) {
        len = strlen(condition);
        len_old = strlen(tc->condition) + 1;
        if(len < len_old) {
            snprintf(tc->condition, len_old + 1, "%s", condition);
        } else {
            free(tc->condition);
            tc->condition = (char*)0;
            cond = malloc(len + 1);
            if(cond == (char*)0) {
                _set_tc_flag_condition_set_failed(tc);
                return tc;
            } else {
                tc->condition = cond;
                snprintf(tc->condition, len + 1, "%s", condition);
            }
        }
    }
    tc->record_num = 0;
    status = find_rows_with_cond_with_ti(tc->ti, tc->condition, &tc->should_free, &record_num);
    if(status != PR_OK) {
        _set_tc_flag_reload_failed(tc);
        return tc;
    }

    if(record_num <= 0 || reload ==(char*)0) {
        _set_tc_flag_query_no_data(tc);
        tc->record = (char*)0;
        return tc;
    }

    memcpy(tc->record, reload, tc->ti->row_size);
    _set_tc_flag_data_loaded(tc);
    free(reload);

    return tc;
}

/* select * from table_name [condition] limit 1 */
LCOM *leader_com_create_by_condition(char *table_name, const char *condition) {
    LCOM *lc = (LCOM *)0;
    INT32 num = 1, status;
    RCOM *rc;

    if(table_name == (char*)0 || table_name[0] == '\0') {
        return lc;
    }

    lc = (LCOM*)malloc(sizeof(LCOM));
    if(lc == (LCOM*)0) {
        return lc;
    }

    lc->leader = table_com_to_row_com(table_com_init_by_tname(table_name));
    rc = lc->leader;

    if(rc == (RCOM*)0) {
        free(lc);
        return (LCOM*)0;
    }

    strncpy(lc->table_name, table_name, sizeof(lc->table_name));

    table_com_load_data(rc, condition);

    return rc;
}
