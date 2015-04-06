#ifndef _PUB_TYPES_H_
#define _PUB_TYPES_H_

#include <inttypes.h>

typedef int8_t    INT8;
typedef int16_t   INT16;
typedef int32_t   INT32;
typedef int64_t   INT64;

typedef int8_t    UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;

typedef float     FLOAT;
typedef double    DOUBLE;
typedef char      CHAR;
typedef char      NCHAR;
typedef char*     STRING;


#define FT_INT8         0x00000001
#define FT_INT16        0x00000002
#define FT_INT32        0x00000004
#define FT_INT64        0x00000008

#define FT_UINT8        0x00000010
#define FT_UINT16       0x00000020
#define FT_UINT32       0x00000040
#define FT_UINT64       0x00000080

#define FT_FLOAT        0x00000100
#define FT_DOUBLE       0x00000200

#define FT_CHAR         0x00001000
#define FT_NCHAR        0x00010000
#define FT_STRING       0x00020000 /* string pointer, should never use */
#define FT_BIN          0x00040000 /* bytes vector, should never use   */

#define FT_SHOW_CHAR(type)     (0x0000F000 & (type))
#define FT_SHOW_STRING(type)   (0x000F0000 & (type))
#define FT_SHOW_INT(type)      (0x000000FF & (type))
#define FT_SHOW_FLOAT(type)    (0x00000F00 & (type))

#define FT_SHOULD_FREE(type)   (FT_STRING  & (type))

typedef struct FT_NAME_MAP{
		INT32		type;
		CHAR* const name;
		CHAR* const sql_name;
}FT_NAME_MAP;



#endif
