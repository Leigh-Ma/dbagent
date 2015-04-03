#ifndef _TABLES_H_
#define _TABLES_H_

#include "table_def.h"
#pragma pack(1)

typedef struct _table_Table {
	int id;
}Table;

/* Free to define the tables as you wish bellow */
/* Do not use key Macros in your comments       */

TABLE_BEGIN(User)
	FIELD  (UINT32,   id              )
	FIELD  (UINT8,    level           )
	FIELD  (UINT8,    vip             )
	FIELD  (UINT16,   exp             )
	FIELD_A(CHAR,     name,       32  )
	FIELD  (CHAR*,    extra           )
	FIELD  (DOUBLE,   pay             )
TABLE_END(User)

TABLE_BEGIN(Server)
	FIELD  (UINT32,  id                )
	FIELD_A(CHAR,    name,         32  )
	FIELD_A(CHAR,    identifier,   32  )
	FIELD_A(CHAR,    category,     16  )
TABLE_END(Server)

#pragma pack(1)

#endif
