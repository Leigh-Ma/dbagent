#ifndef _TABLE_QUERY_H_
#define _TABLE_QUERY_H_

#define QUERY_MAX_SQL_LEN 2048


#define _get_ti_by_tname(name, ti)                                          \
    ti = query_get_table_info_by_name(name);                                \
    _CHECK_RET(ti != (TI*)0, ERR_TBL_NAME);                                 \
    _CHECK_RET(ti->bad_fnames == 1 || ti->bad_fnames == 0, ERR_FLD_SQL );   \


#define _get_ti_by_tno(tno, ti)                                             \
    ti = query_get_table_info_by_tno(tno);                                  \
    _CHECK_RET(ti != (TI*)0, ERR_TBL_NO);                                   \
    _CHECK_RET(ti->bad_fnames == 1 || ti->bad_fnames == 0, ERR_FLD_SQL );   \


#define FIND(table, id, pprows, pnum, status)                               \
    {                                                                       \
        char condition[32]={0};                                             \
        snprintf(condition, 32, "where id = %llu", id);                     \
        status = find_rows_with_cond_with_tno(TNO_##table, condition ,      \
                                                        pprows, pnum);      \
    }

#define FIND_WITH_INT(table, field, value, pprows, pnum, status)            \
    {                                                                       \
        char condition[32]={0};                                             \
        snprintf(condition, 32, "where %s = %llu", field, value);           \
        status = find_rows_with_cond_with_tno(TNO_##table, condition ,      \
                                                        pprows, pnum);      \
    }

#define FIND_WITH_STR(table, field, value, pprows, pnum, status)            \
    {                                                                       \
        char condition[64]={0};                                             \
        snprintf(condition, 32, "where %s = '%s'", field, value);           \
        status = find_rows_with_cond_with_tno(TNO_##table, condition ,      \
                                                        pprows, pnum);      \
    }

#endif
