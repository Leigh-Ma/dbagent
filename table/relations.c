#include "table_pub.h"

TCOM *table_com_set_condition(TCOM* tc, const char *condition) {
    TCOM *belong = (TCOM*)0;

    if( tc==(TCOM*)0 ) {
        return (TCOM*)0;
    }

    _set_tc_flag_error(tc, "condition");
    if(condition == (char*)0 || *condition == '\0') {
        if(condition !=(char*)0 && *condition == '\0') {
            if(tc->condition !=(char*)0) {
               free(tc->condition);
               tc->condition = (char*)0;
            }
        }
        belong = tc->belong;
        if( (belong != (TCOM*)0) && (belong->record != (char*)0) && (tc->condition == (char*)0) ) {
            tc->condition = (char*) malloc(32);
            if(tc->condition == (char*)0) {
                return tc;
            }
            snprintf(tc->condition, 32, "where %s = %d", belong->ti->far_id, *(UINT32*)belong->record);
        }
    } else {
        if(tc->condition != (char*)0) {
            free(tc->condition);
            tc->condition = (char*)0;
        }
        tc->condition = (char*) malloc(strlen(condition) + 32);
        if(tc->condition == (char*)0) {
            return tc;
        }
        belong = tc->belong;
        if(belong != (TCOM*)0 && belong->record != (char*)0) {
            snprintf(tc->condition, 32, "where %s = %d and (%s)", belong->ti->far_id, *(UINT32*)belong->record, condition);
        } else {
            snprintf(tc->condition, 32, "where %s", condition);
        }
    }
    _clr_tc_flag_error(tc, "condition");

    return tc;
}

TCOM *table_com_init_by_ti(TI *ti) {
    TCOM *tc = (TCOM*)0;

    _CHECK_PARAMS_RET(ti != (TI*)0, tc);

    tc = (TCOM*) malloc(sizeof(TCOM));
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


/* use after release data, or right after initialization,
 * @tc will be group leader after use
 * should be used only when @tc's group leader
 */
TCOM *table_com_load_data(TCOM* tc, const char *condition) {
    INT32 status = PR_OK;
    TCOM  *more_tc, *last_tc;

    if(tc == (TCOM*)0) {
        return tc;
    }

    _set_tc_flag_attr(tc, "group");             /* group leader record */

    table_com_set_condition(tc, condition);
    if(_get_tc_flag_error(tc, "condition")) {
        return tc;
    }

    _set_tc_flag_error(tc, "query");

    tc->record_num = tc->query_num;
    status = find_rows_with_cond_with_ti(tc->ti, tc->condition, &tc->should_free, &tc->record_num);

    if(status != PR_OK || tc->should_free == (char*)0) {
        return tc;
    }

    _clr_tc_flag_error(tc, "query");


    if(tc->record_num <= 0 || tc->should_free == (char*)0) {
        _set_tc_flag_attr(tc, "no_data");
        return tc;
    }

    tc->record = tc->should_free;
    _set_tc_flag_attr(tc, "data");

    last_tc = tc;
    for(status = 1; status < tc->record_num; status++) {
        if(tc->recycle) {
            more_tc     = tc->recycle;
            tc->recycle = more_tc->next;
        } else {
            more_tc     = table_com_init_by_ti(tc->ti);
        }

        if(more_tc == (TCOM*)0) {
            _set_tc_flag_error(tc, "new rows");
        } else {
            more_tc->record_num = tc->record_num;
            more_tc->record     = tc->record + tc->ti->row_size;
            last_tc->next       = more_tc;
            last_tc             = more_tc;
            _set_tc_flag_attr(more_tc, "data");
        }
    }

    return tc;
}

/*
 * release data in table compress set, @tc should be leader
 * do not deal with has chain
 */
TCOM *table_com_release_data(TCOM* tc) {
    TCOM  *next;

    if(tc == (char*)0 || !_get_tc_flag_attr(tc, "group")) {
        return tc;
    }

    next = tc->next;
    while(next) {
        next->record     = (char*)0;
        next->record_num = 0;
        _clr_tc_flags(next);

        tc->next         = next->next;          /* delete @next from tc->next list  */
        next->next       = tc->recycle;
        tc->recycle      = next;                /* prepend @next to recycle         */
        next             = next->next;
    }

    tc->record = (char*)0;
    tc->record_num = 0;
    _clr_tc_flags(tc);

    if(tc->should_free != (char*)0) {
        free(tc->should_free);
        tc->should_free = (char*)0;
    }
    if(tc->condition != (char*)0) {
        free(tc->condition);                    /* TODO: reuse condition buffer  */
        tc->condition = (char*)0;
    }

    _set_tc_flag_attr(tc, "group");
    return tc;
}

TCOM *table_com_reload_data(TCOM* tc, const char *condition) {
    return table_com_load_data(table_com_release_data(tc), condition);
}

/*
 * free all the memory resource used by table data compress,
 * including @tc it's self, and @tc should be leader
 */
TCOM *table_com_destroy(TCOM* tc) {
    TCOM  *next, *last_has, *has;

    if(tc == (char*)0 || !_get_tc_flag_attr(tc, "group")) {
        return tc;
    }

    for(next = tc; next != (TCOM*)0; next = next->next) {
        /* if these group of row belong to a certain row, destroy the relation link */
        table_rows_show(next->ti->name, next->record, 1);
        if(next->has) {
            table_com_destroy(next->has);
        }
        if(next->has_next) {
            table_com_destroy(next->has_next);
        }

        if(next->belong) {
            last_has = next->belong->has;
            for(has = next->belong->has; has != (TCOM*)0; has = has->has_next) {
                if(has->table_no == tc->table_no) {
                    if(last_has == next->belong->has) {
                        next->belong->has = (TCOM*)0;
                    } else {
                        last_has->has_next = has->has_next;
                    }
                    break;
                }
                last_has = has;
            }
        }

        if(next->should_free != (char*)0) {
            table_rows_release_by_ti(next->ti, next->should_free, next->record_num);
        }
        if(next->condition != (char*)0) {
            free(next->condition);                    /* TODO: reuse condition buffer  */
        }
        free(next);
    }


    return (TCOM*)0;
}


RCOM *table_com_to_row_com(TCOM *tc) {
    if(tc == (TCOM*)0 ) {
       return tc;
    }

    tc->query_num = 1;
    _set_tc_flag_attr(tc, "group");         /* group leader */

    return tc;
}

TCOM *table_com_show_data(TCOM* tc) {
    TCOM *more_tc = tc;

    for(; more_tc != (TCOM*)0; more_tc = more_tc->next) {
        row_com_show_data(more_tc);
    }
    return tc;
}

TCOM *table_com_find(TCOM *tc, table_com_satisfy satisfy) {
    TCOM *row;

    for(row = tc; row != (TCOM*)0; row = row->next) {
        if(satisfy(row)) {
            break;
        }
    }

    return row;
}

TCOM *row_com_show_data(TCOM* tc) {
    if(tc == (TCOM*)0) {
        return (TCOM*)0;
    }
    table_rows_show(tc->ti->name, tc->record, 1);
    return tc;
}

/* reload current row only */
TCOM *row_com_reload_data(TCOM* tc) {
    INT32 status = PR_OK, query_num = 1;
    char  condition[32] = {0};
    void  *reload;

    if(tc == (TCOM*)0 || tc->record == (char*)0) {
        return tc;
    }

    snprintf(condition, 32, "where id = %d", *(UINT32*)tc->record);

    _set_tc_flag_error(tc, "query");
    status = find_rows_with_cond_with_ti(tc->ti, condition, &reload, &query_num);
    if(status != PR_OK) {
        return tc;
    }
    _clr_tc_flag_error(tc, "query");

    if(query_num <= 0 || reload == (char*)0) {
        _set_tc_flag_attr(tc, "no_data");
        tc->record = (char*)0;
        return tc;
    }

    memcpy(tc->record, reload, tc->ti->row_size);
    _set_tc_flag_attr(tc, "data");
    free(reload);

    return tc;
}

TCOM *row_com_find_or_create_has(TCOM* tc, const char *table_name) {
    TCOM *has;

    if(tc == (TCOM*)0 || table_name == (char*)0) {
        return (TCOM*)0;
    }

    for(has = tc->has; has != (TCOM*)0; has = has->has_next) {
        if(0 == strcmp(has->ti->name, table_name)) {
            break;
        }
    }

    if(has != (TCOM*)0) {
        has = table_com_release_data(has);
    } else {
        has = table_com_init_by_tname(table_name);
    }

    if(has != (TCOM*)0) {
        has->has_next = tc->has;        /* prepend to the list              */
        tc->has = has;                  /* tc->has is the head of the list  */
        has->belong = tc;
    }

    return has;
}

/* query another table_com rows that belongs to current row, no count limits */
TCOM *row_com_has_table_com(TCOM* tc, const char *table_name, const char *condition) {
    TCOM  *has   = (TCOM*)0;

    if(tc == (TCOM*)0 || tc->record == (char*)0) {
        return tc;
    }

    has = row_com_find_or_create_has(tc, table_name);
    _CHECK_RET(has != (TCOM*)0, (TCOM*)0);

    table_com_set_condition(has, condition);
    if(_get_tc_flag_error(has, "condition")) {
        return tc;
    }

    return table_com_load_data(has, (char*)0);
}

/*
 * select * from table_name [condition] limit 1
 * create a LCOM by table name and condition
 */
LCOM *leader_com_create_by_condition(char *table_name, const char *condition) {
    LCOM *lc = (LCOM *)0;
    RCOM *rc;

    if(table_name == (char*)0 || table_name[0] == '\0') {
        return lc;
    }

    lc = (LCOM*)malloc(sizeof(LCOM));
    if(lc == (LCOM*)0) {
        return lc;
    }

    lc->leader = table_com_init_by_tname(table_name);
    rc = lc->leader;

    if(rc == (RCOM*)0) {
        free(lc);
        return (LCOM*)0;
    }

    snprintf(lc->table_name, sizeof(lc->table_name), "%s", table_name);

    table_com_to_row_com(rc);
    table_com_load_data(rc, condition);

    return lc;
}
