#ifndef _AGT_TYPES_H_
#define _AGT_TYPES_H_

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
typedef char*     STR;


#define FT_INT8         0x00000001
#define FT_INT16        0x00000002
#define FT_INT32        0x00000004
#define FT_INT64        0x00000008
#define FT_FLOAT        0x00000010
#define FT_DOUBLE       0x00000020

#define FT_CHAR         0x00000100
#define FT_NCHAR        0x00000200
#define FT_STR          0x00000400 /* string pointer, should never use */
#define FT_BIN          0x00000800 /* bytes vector, should never use   */

#define FT_UINT8        0x00001000
#define FT_UINT16       0x00002000
#define FT_UINT32       0x00004000
#define FT_UINT64       0x00008000

#endif
