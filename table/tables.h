#ifndef _TABLES_H_
#define _TABLES_H_

#include "table_pub.h"
#pragma pack(1)

extern TI* 		g_all_tables[];
extern INT32 	g_all_tables_num;

/* Free to define the tables as you wish bellow */
/* Do not use key Macros in your comments       */

TABLE_BEGIN(User)
	FIELD  (UINT32,   id              )
	FIELD  (UINT8,    level           )
	FIELD  (UINT8,    vip             )
	FIELD  (UINT16,   exp             )
	FIELD_A(NCHAR,    name,       32  )
	FIELD  (STRING,   extra           )
	FIELD  (DOUBLE,   pay             )
TABLE_END(User)

TABLE_BEGIN(Server)
	FIELD  (UINT32,   id                )
	FIELD_A(NCHAR,    name,         32  )
	FIELD_A(NCHAR,    identifier,   32  )
	FIELD_A(NCHAR,    category,     16  )
TABLE_END(Server)

#pragma pack(1)

#endif
