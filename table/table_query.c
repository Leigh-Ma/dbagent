#include "table_pub.h"


const TI *query_get_table_info_by_name(const char *table_name) {
    int i = TNO_MINIMUM;

    _CHECK_PARAMS_RET(
         (const char *)0 != table_name,
         (TI*)0
    )

    for(; i <= TNO_MAXIMUM; i++) {
        if( 0 == strncmp(table_name, g_all_tables_info[i]->name, sizeof(g_all_tables_info[i]->name)) ||
            0 == strncmp(table_name, g_all_tables_info[i]->name_ex, sizeof(g_all_tables_info[i]->name_ex))) {
            return g_all_tables_info[i];
        }
    }

    logger("query: can not find table info with name: %s", table_name);
    return (TI*)0;
}


const TI *query_get_table_info_by_tno(const int tno) {
    _CHECK_PARAMS_RET(
        TNO_MINIMUM <= tno && TNO_MAXIMUM >= tno,
        (TI*)0
    )

    return g_all_tables_info[tno];
}

/* select query_get_table_info_by_name(@table_name)->fnames from @table_name condition_with_where; */
int find_rows_with_cond_with_tname(const char *table_name, const char *condition_with_where ,
        _FREE_ void **rows, int *num) {

    TI *ti;

    _CHECK_PARAMS_RET(
        table_name && rows,
        ERR_PARAM
    )

    _get_ti_by_tname(table_name, ti);

    return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);

}

int find_rows_with_cond_with_tno(const int tno, const char *condition_with_where , _FREE_ void **rows, int *num) {
    TI *ti;

    _CHECK_PARAMS_RET(
         num != (int *)0 && rows != (void **)0,
         ERR_PARAM
    )

    _get_ti_by_tno(tno, ti);

    return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_all_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num) {
    TI *ti;

    _CHECK_PARAMS_RET(
        num != (int *)0 && rows != (void **)0,
        ERR_PARAM
    )

    *num = 0;
    _get_ti_by_tno(tno, ti);

    return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_all_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num) {
    TI *ti;

    _CHECK_PARAMS_RET(
        num != (int *)0 && rows != (void **)0,
        ERR_PARAM
    );

    *num = 0;
    _get_ti_by_tname(table_name, ti);

    return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num) {
    TI *ti;

    _CHECK_PARAMS_RET(
        num != (int *)0 && rows != (void **)0,
        ERR_PARAM
    );

    *num = 0;
    _get_ti_by_tno(tno, ti);
    *num = 1;
    return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num) {
    TI *ti;

    _CHECK_PARAMS_RET(
        num != (int *)0 && rows != (void **)0,
        ERR_PARAM
    )

    *num = 0;
    _get_ti_by_tname(table_name, ti);
    *num = 1;
    return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}


int find_rows_with_cond_with_ti(const TI *ti, const char *condition_with_where , _FREE_ void **rows, int *num) {
    const char *condition_ex = "";
    char *condition = condition_with_where ? condition_with_where : condition_ex;
    char sql[QUERY_MAX_SQL_LEN] = {0};;
    char *head, *tail, *raw, *p = (char*)0;
    int row_num = 0, buff_size, status = 0;

    _CHECK_PARAMS_RET(
        ti != (TI*)0 && num != (int *)0 && rows != (void **)0,
        ERR_PARAM
    )

    p = ti->bad_fnames == 0 ? ti->field_names : ti->far_names;
    if(*num <= 0) {
        snprintf(sql, QUERY_MAX_SQL_LEN, "select  %s from %s %s;", p, ti->name, condition);
    } else {
        snprintf(sql, QUERY_MAX_SQL_LEN, "select  %s from `%s` %s limit %d;", p, ti->name, condition, *num);
    }

    _CHECK_RET_EX(
        sql[QUERY_MAX_SQL_LEN-2] == '\0',
        ERR_SQL_LEN,
        sql
    )

    do_query(sql, &head, num);
    row_num = *num;
    p = head;

    logger("find %d record(s): %s\n",*num, head);
    if(*num == 0 || p == (void*)0) {
        *rows = (void*)0;
        return status;
    }

    /* has some return rows */
    buff_size = row_num * ti->row_size;
    *rows = malloc(buff_size);
    raw = (char*)*rows;
    if(raw == (char*) 0) {
        status = ERR_SYS_MEM;
    }
    /* begin to parse result from string representation */

    while( (status == 0) && (head != (char*)0) && (0 < row_num--) ) {
        status = parse_row_from_str(head, ti->tfs, ti->tfn, buff_size, raw, &tail);
        buff_size -= ti->row_size;
        raw  += ti->row_size;
        head = tail;
    }

    if(status != 0) {
        if(*rows) {
            free(*rows);
            *rows = (void*)0;
        }
        *num = 0;
    }
    return status;
}


