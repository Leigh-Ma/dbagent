#include <inttypes.h>
#include "../pub/agt_types.h"
/* table data phrase APIs  */

/* all right               */
#define PR_OK               0

/* params error            */
#define PR_ERR_PARAM        1

/* field num error         */
#define PR_ERR_FNUM         2

/* field data length error */
#define PR_ERR_FLEN         3

 /* row data length error   */
#define PR_ERR_RLEN         4

/* data error              */
#define PR_ERR_DATA         5

/* field type error */
#define PR_ERR_FTYPE        6

#define _SPI_              '|'       /* filed value split identifier*/
#define _prase_result(val)               val
#define _prase_result_ex(val, msg)       val

#define _prase_warning(val)              val
#define _prase_warning_ex(val, msg)      msg, val


#define _prase_begin(f)                             \
	    switch(f->type) {

#define _prase_try(T, F, psrc, pdst, plen)          \
		case FT_##T :                               \
			_AS_##T(F, psrc, pdst, plen);           \
			break;
#define _prase_end                                  \
	    default:                                    \
	        return _prase_result(PR_ERR_FTYPE);     \
	   }

#define _declare_prase_as_ints(category, digits)                            \
static inline void                                                          \
_AS_##category##digits(const TF *f, const char *p, char *buff, int *len) {  \
	assert(1 == f->num);                                                    \
	*(category##digits *) buff = (category##digits) aotll(p);               \
	*len = sizeof(category##digits);                                        \
}

_declare_prase_as_ints(INT, 8)
_declare_prase_as_ints(INT, 16)
_declare_prase_as_ints(INT, 32)
_declare_prase_as_ints(INT, 64)

_declare_prase_as_ints(UINT, 8)
_declare_prase_as_ints(UINT, 16)
_declare_prase_as_ints(UINT, 32)
_declare_prase_as_ints(UINT, 64)

static inline void _AS_CHAR (const TF *f, const char *p, char *buff, int *len) {
	assert(1 == f->num && 1 == f->size);
	*(CHAR *) buff = *((CHAR*)p);
	*len = 1;
}

static inline void _AS_NCHAR(const TF *f, const char *p, char *buff, int *len) {
	int i = 0;

	assert(f->num == f->size && 1 == f->base_size);

	while(f->num > i && *p != _SPI_ ) {
		buff[i] = *p;
		p++, i++;
	}
	buff[i >= f->num ? i-1 : i] = '\0';
	*len = f->size;
}

/*should never use, to avoid memory leak*/
static inline void _AS_STR  (const TF *f, const char *p, char *buff, int *len) {
	char *sp = strchr(p, _SPI_), *danger;
	int  length;

	assert(f->size == sizeof(char*) && sp != (char*)0);
	length = (int)(sp - p);
	if(length == 0) {
		danger = (char *)0;
	}else{
		danger = (char*)malloc(length + 1);
		if(danger) {
			memcpy(danger, p, length);
			danger[length] = '\0';
		}
	}
	*(char**)buff = danger;
	*len = sizeof(char*);
}

static inline void _AS_BIN  (const TF *f, const char *p, char *buff, int *len) {
	/*not supported*/
	assert(0==1);
}

static inline void _AS_FLOAT  (const TF *f, const char *p, char *buff, int *len) {
	assert(f->size == sizeof(FLOAT) && f->num == 1);

	*(FLOAT*)buff = atof(p);
	*len = sizeof(FLOAT);
}

static inline void _AS_DOUBLE  (const TF *f, const char *p, char *buff, int *len) {
	assert(f->size == sizeof(DOUBLE) && f->num == 1);

	*(DOUBLE*)buff = atof(p);
	*len = sizeof(DOUBLE);
}

/* function bellow should be exported */
int prase_field_from_str(const TF *f, const char *p, int ex, char *buff, int *len) {

	int elem_num;

	if(*len < f->size) {
		return _prase_result(PR_ERR_RLEN);
	}

	_prase_begin(f)
		_prase_try(INT8,    f, p,  buff,  len)
		_prase_try(INT16,   f, p,  buff,  len)
		_prase_try(INT32,   f, p,  buff,  len)
		_prase_try(INT64,   f, p,  buff,  len)

		_prase_try(FLOAT,   f, p,  buff,  len)
		_prase_try(DOUBLE,  f, p,  buff,  len)

		_prase_try(CHAR,    f, p,  buff,  len)
		_prase_try(NCHAR,   f, p,  buff,  len)
		_prase_try(STR,     f, p,  buff,  len)
		_prase_try(BIN,     f, p,  buff,  len)

		_prase_try(UINT8,   f, p,  buff,  len)
		_prase_try(UINT16,  f, p,  buff,  len)
		_prase_try(UINT32,  f, p,  buff,  len)
		_prase_try(UINT64,  f, p,  buff,  len)
	_prase_end

	return PR_OK;
}

int prase_row_from_str(const char *row, const TF *tfs, const int tfn, Table *record, int *len) {
	int  f_len, r_len = 0, i = -1, ret = PR_OK;
	char *p = row, *r = (char*)record;
	TF   *f = tfs;

	if(p == (char*)0 || f == (TF*)0 || tfn == 0 ){
		return _prase_result(PR_ERR_PARAM);
	}

	memset(r, 0x00, *len);
	while(i < tfn && (char *)0 != (p = strchr(p, _SPI_))) {
		f_len = 0; p++;
		if(i == -1 ) {
			if (!tfn == ato_i(p)) {
				return _prase_result(PR_ERR_FLEN);
			}
			i++;
			continue;
		}
		f_len = *len - r_len;
		if(PR_OK != (ret = prase_field_from_str(f, p, *len, r + r_len, &f_len))) {
			return _prase_result(ret);
		}
		r_len += f_len;
	}

	if(i != tfn) {
		return _prase_result(PR_ERR_DATA);
	}
	if(*len != r_len) {
		return _prase_warning_ex(PR_ERR_RLEN, *len = r_len);
	}
	*len = r_len;
	return PR_OK;
}
