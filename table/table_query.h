#ifndef _TABLE_QUERY_H_
#define _TABLE_QUERY_H_

#define QUERY_MAX_SQL_LEN 2048


#define _get_ti_by_tname(name, ti)											\
	ti = query_get_table_info_by_name(name);								\
	_CHECK_RET(ti != (TI*)0, PR_ERR_TABLE);									\
	_CHECK_RET(ti->bad_fnames == 1 || ti->bad_fnames == 0, PR_ERR_FLEN );	\


#define _get_ti_by_tno(tno, ti)												\
	ti = query_get_table_info_by_tno(tno);									\
	_CHECK_RET(ti != (TI*)0, PR_ERR_TABLE);									\
	_CHECK_RET(ti->bad_fnames == 1 || ti->bad_fnames == 0, PR_ERR_FLEN );	\


#define FIND(table, id, pprows, pnum, status)								\
	{																		\
		char condition[32]={0};												\
		snprintf(condition, 32, "where id = %llu", id);						\
		status = find_rows_with_cond_with_tno(TNO_##table, condition , 		\
														pprows, pnum);		\
	}



#endif
