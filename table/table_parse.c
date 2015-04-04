#include "table_pub.h"


/* table data parse APIs  */
const struct FT_NAME_MAP g_type_name_map[] = {
			{FT_CHAR,   	"CHAR"},
			{FT_NCHAR,   	"NCHAR"},
			{FT_INT8,		"INT8"},
			{FT_INT16, 		"INT16"},
			{FT_INT32, 		"INT32"},
			{FT_INT64, 		"INT64"},

			{FT_UINT8,		"UINT8"},
			{FT_UINT16, 	"UINT16"},
			{FT_UINT32, 	"UINT32"},
			{FT_UINT64, 	"UINT64"},

			{FT_FLOAT, 		"FLOAT"},
			{FT_DOUBLE,		"DOUBLE"},

			{FT_STRING,		"STRING"},
	};

static const int _f_type_num = sizeof(g_type_name_map)/sizeof(g_type_name_map[0]);


int parse_field_from_str(const TF *f, const char *p, int ex, char *buff, int *len) {

	if(*len < f->size) {
		return _parse_result(PR_ERR_RLEN);
	}

	_parse_begin(f)
		_parse_try(INT8,    f, p,  buff,  len)
		_parse_try(INT16,   f, p,  buff,  len)
		_parse_try(INT32,   f, p,  buff,  len)
		_parse_try(INT64,   f, p,  buff,  len)

		_parse_try(FLOAT,   f, p,  buff,  len)
		_parse_try(DOUBLE,  f, p,  buff,  len)

		_parse_try(CHAR,    f, p,  buff,  len)
		_parse_try(NCHAR,   f, p,  buff,  len)
		_parse_try(STRING,  f, p,  buff,  len)
		_parse_try(BIN,     f, p,  buff,  len)

		_parse_try(UINT8,   f, p,  buff,  len)
		_parse_try(UINT16,  f, p,  buff,  len)
		_parse_try(UINT32,  f, p,  buff,  len)
		_parse_try(UINT64,  f, p,  buff,  len)
	_parse_end

	return PR_OK;
}

int parse_row_from_str(const char *row, const TF *tfs, const int tfn, char *record, int *len) {
	int  f_len, r_len = 0, i = -1, ret = PR_OK;
	char *p = row, *r = (char*)record;
	TF   *f = tfs;

	if(p == (char*)0 || f == (TF*)0 || tfn == 0 ){
		return _parse_result(PR_ERR_PARAM);
	}

	memset(r, 0x00, *len);
	while(i < tfn && (char *)0 != (p = strchr(p, _SPI_))) {
		f_len = 0; p++;
		if(i == -1 ) {
			if (!tfn == ato_i(p)) {
				return _parse_result(PR_ERR_FLEN);
			}
			i++;
			continue;
		}
		f_len = *len - r_len;
		if(PR_OK != (ret = parse_field_from_str(f, p, *len, r + r_len, &f_len))) {
			return _parse_result(ret);
		}
		r_len += f_len;
	}

	if(i != tfn) {
		return _parse_result(PR_ERR_DATA);
	}
	if(*len != r_len) {
		return _parse_warning_ex(PR_ERR_RLEN, *len = r_len);
	}
	*len = r_len;
	return PR_OK;
}


char const *parse_field_type_name(INT32 type) {
	struct FT_NAME_MAP *pt = g_type_name_map;
	int i;

	for(i = 0; i < _f_type_num; i++, pt++) {
		if(type == pt->type) {
			return pt->name;
		}
	}
	assert(0 == 1);
	return "INVALID_TYPE";
}

