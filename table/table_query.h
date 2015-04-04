#ifndef _TABLE_QUERY_H_
#define _TABLE_QUERY_H_

#include "table_pub.h"

#define QUERY_MAX_SQL_LEN 2048


#define _get_ti_by_name(name, ti)											\
	ti = query_get_table_info_by_name(table_name);							\
	_CHECK_RET(ti != (TI*)0, PR_ERR_TABLE);									\
	_CHECK_RET(ti->bad_fnames == 1 || ti->bad_fnames == 0, PR_ERR_FLEN );	\


#define _get_ti_by_tno(tno, ti)												\
	ti = query_get_table_info_by_name(tno);									\
	_CHECK_RET(ti != (TI*)0, PR_ERR_TABLE);									\
	_CHECK_RET(ti->bad_fnames == 1 || ti->bad_fnames == 0, PR_ERR_FLEN );	\


#define FIND(table, condtion , ret)

const TI *query_get_table_info_by_name(const char *table_name);

const TI *query_get_table_info_by_tno(const int tno);

int find_rows_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num);

int find_rows_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num);

int find_all_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num);

int find_all_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num);

int find_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num);

int find_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num);


#endif
