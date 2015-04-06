#include "table_pub.h"

/* table data parse APIs  */
const struct FT_NAME_MAP g_type_name_map[] = {
			{FT_CHAR,   	"CHAR", 	"char(1)" },
			{FT_NCHAR,   	"NCHAR", 	"char"    },
			{FT_INT8,		"INT8", 	"tinyint" },
			{FT_INT16, 		"INT16", 	"smallint"},
			{FT_INT32, 		"INT32", 	"int"     },
			{FT_INT64, 		"INT64", 	"bigint"  },

			{FT_UINT8,		"UINT8", 	"tinyint" },
			{FT_UINT16, 	"UINT16",	"smallint"},
			{FT_UINT32, 	"UINT32", 	"int"     },
			{FT_UINT64, 	"UINT64",   "bigint"  },

			{FT_FLOAT, 		"FLOAT",    "float"   },
			{FT_DOUBLE,		"DOUBLE", 	"double"  },

			{FT_STRING,		"STRING", 	"varchar(255)"},
	};

const int g_field_type_num = sizeof(g_type_name_map)/sizeof(g_type_name_map[0]);


int parse_field_from_str(const TF *f, const char *p, int len, char *buff) {
	int field_len = 0;

	if(len < f->size) {
		return _parse_result(PR_ERR_RLEN);
	}

	_parse_begin(f)
		_parse_try(INT8,    f, p,         buff,  &field_len);
		_parse_try(INT16,   f, p,         buff,  &field_len)
		_parse_try(INT32,   f, p,         buff,  &field_len)
		_parse_try(INT64,   f, p,         buff,  &field_len)

		_parse_try(FLOAT,   f, p,         buff,  &field_len)
		_parse_try(DOUBLE,  f, p,         buff,  &field_len)

		_parse_try(CHAR,    f, p,         buff,  &field_len)
		_parse_try(NCHAR,   f, p,         buff,  &field_len)
		_parse_try(STRING,  f, p, (char**)buff,  &field_len)
		_parse_try(BIN,     f, p,         buff,  &field_len)

		_parse_try(UINT8,   f, p,         buff,  &field_len)
		_parse_try(UINT16,  f, p,         buff,  &field_len)
		_parse_try(UINT32,  f, p,         buff,  &field_len)
		_parse_try(UINT64,  f, p,         buff,  &field_len)
	_parse_end

	return PR_OK;
}

int parse_row_from_str(const char *row, const TF *tfs, int tfn, int len, char *record, char **tail) {
	int  r_len = 0, i = -1, ret = PR_OK;
	char *p = row, *r = (char*)record;
	TF   *f = tfs;

	if(p == (char*)0 || f == (TF*)0 || tfn == 0 ){
		return _parse_result(PR_ERR_PARAM);
	}

	while(i < tfn && (char *)0 != (p = strchr(p, _SPI_))) {
	    p++;
		if(i == -1 ) {
			if (tfn != atoi(p)) {
				return _parse_result(PR_ERR_FLEN);
			}
			i++;
			continue;
		}
		if(PR_OK != (ret = parse_field_from_str(f, p, len-r_len, r + r_len))) {
			return _parse_result(ret);
		}
		r_len += f->size;
		f++, i++;
	}

	if(i != tfn) {
		return _parse_result(PR_ERR_DATA);
	}

	*tail = strchr(p, _SPI_);
	return PR_OK;
}


char const *parse_field_type_name(INT32 type) {
	struct FT_NAME_MAP *pt = g_type_name_map;
	int i;

	for(i = 0; i < g_field_type_num; i++, pt++) {
		if(type == pt->type) {
			return pt->name;
		}
	}
	assert(0 == 1);
	return "INVALID_TYPE";
}

char const *parse_field_type_sql_name(INT32 type) {
	struct FT_NAME_MAP *pt = g_type_name_map;
	int i;

	for(i = 0; i < g_field_type_num; i++, pt++) {
		if(type == pt->type) {
			return pt->sql_name;
		}
	}
	assert(0 == 1);
	return "INVALID_TYPE";
}

