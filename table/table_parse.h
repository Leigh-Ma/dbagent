#ifndef _TABLE_PARSE_H_
#define _TABLE_PARSE_H_

#define _SPI_                           '|'         /* filed value split identifier*/



#define _parse_begin(f)                             \
    switch(f->type) {

#define _parse_try(T, F, psrc, pdst, plen)          \
    case FT_##T :                                   \
        _AS_##T(F, psrc, pdst, plen);               \
        break;

#define _parse_end                                  \
    default:                                        \
        return ERR_FLD_TYPE;                        \
    }

#define _declare_parse_as_ints(category, digits)                            \
static inline void                                                          \
_AS_##category##digits(const TF *f, const char *p, char *buff, int *len) {  \
    assert(1 == f->num);                                                    \
    *(category##digits *) buff = (category##digits) atoll(p);               \
    *len = sizeof(category##digits);                                        \
}


_declare_parse_as_ints(INT,  8)
_declare_parse_as_ints(INT,  16)
_declare_parse_as_ints(INT,  32)
_declare_parse_as_ints(INT,  64)

_declare_parse_as_ints(UINT, 8)
_declare_parse_as_ints(UINT, 16)
_declare_parse_as_ints(UINT, 32)
_declare_parse_as_ints(UINT, 64)

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
static inline void _AS_STRING(const TF *f, const char *p, char **buff, int *len) {
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

    *buff = danger;
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

/************************************************************************************/
#endif
