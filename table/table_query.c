#include "table_pub.h"

static int find_rows_with_cond_with_ti(const TI *ti, const char *condition_with_where ,
		_FREE_ void **rows, int *num);

const TI *query_get_table_info_by_name(const char *table_name) {
	int i = TNO_MINIMUM;

	_CHECK_RET_EX((const char *)0 != table_name, (TI*)0, "query: table name should not be null");

	for(; i <= TNO_MAXIMUM; i++) {
		if( !strcmp(table_name, g_all_tables_info[i]->name) ||
			!strcmp(table_name, g_all_tables_info[i]->name_ex)) {
			return g_all_tables_info[i];
		}
	}

	logger("query: can not find table info with name: %s", table_name);
	return (TI*)0;
}


const TI *query_get_table_info_by_tno(const int tno) {
	CHECK_RET_EX(TNO_MINIMUM <= tno && TNO_MAXIMUM >= tno, (TI*)0, "query: table no not in valid range");

	return g_all_tables_info[tno];
}


/* select query_get_table_info_by_name(@table_name)->fnames from @table_name condition_with_where; */
int find_rows_with_cond_with_tname(const char *table_name, const char *condition_with_where ,
		_FREE_ void **rows, int *num) {

	TI *ti;

	_CHECK_RET(table_name && rows, PR_ERR_PARAM);

	_get_ti_by_name(table_name, ti);

	return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);

}

int find_rows_with_cond_with_tno(const int tno, const char *condition_with_where , _FREE_ void **rows, int *num) {
	TI *ti;

	_CHECK_RET(num != (int *)0 && rows != (void **)0, PR_ERR_PARAM);

	_get_ti_by_tno(tno, ti);

	return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_all_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num) {
	TI *ti;

	_CHECK_RET(num != (int *)0 && rows != (void **)0, PR_ERR_PARAM);

	*num = 0;
	_get_ti_by_tno(tno, ti);

	return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_all_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num) {
	TI *ti;

	_CHECK_RET(num != (int *)0 && rows != (void **)0, PR_ERR_PARAM);

	*num = 0;
	_get_ti_by_tname(table_name, ti);

	return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num) {
	TI *ti;

	_CHECK_RET(num != (int *)0 && rows != (void **)0, PR_ERR_PARAM);

	*num = 0;
	_get_ti_by_tno(tno, ti);
	*num = 1;
	return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}

int find_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num) {
	TI *ti;

	_CHECK_RET(num != (int *)0 && rows != (void **)0, PR_ERR_PARAM);

	*num = 0;
	_get_ti_by_tname(table_name, ti);
	*num = 1;
	return find_rows_with_cond_with_ti(ti, condition_with_where, rows, num);
}


static int find_rows_with_cond_with_ti(const TI *ti, const char *condition_with_where , _FREE_ void **rows, int *num) {
	const char *condition_ex = "where 1 = 1";
	char *condition = condition_with_where ? condition_with_where : condition_ex;
	char sql[QUERY_MAX_SQL_LEN] = {0}, *list = (char*)0;
	char *row_str;
	int row_num = 0;

	list = ti->bad_fnames == 0 ? ti->field_names : ti->far_names;
	if(*num <= 0) {
		snprintf(sql, QUERY_MAX_SQL_LEN, "select %s from %s %s", list, ti->name, condition);
	} else {
		snprintf(sql, QUERY_MAX_SQL_LEN, "select %s from %s %s limit %d", list, ti->name, condition, *rows);
	}

	_CHECK_RET_EX(sql[QUERY_MAX_SQL_LEN-2] == '\0', PR_ERR_SLEN, sql);

	do_query(sql, rows, num);
	row_num = *num;

	if(*num > 0) {
		*rows = malloc(*num * ti->row_size);
	}
	while(*rows && 0 < row_num--) {
		parse_row_from_str(rows, ti->tfs, ti->tfn, *rows, row_num * ti->row_size );
	}
	if(*rows == (void *)0) {
		*num = 0;
	}

	return PR_OK;
}
