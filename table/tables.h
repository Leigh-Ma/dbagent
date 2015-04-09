#ifndef _TABLES_H_
#define _TABLES_H_

#pragma pack(1)

/* Free to define the tables as you wish bellow */
/* Do not use key Macros in your comments       */

TABLE_BEGIN(User)
    FIELD  (UINT32,   id                )
    FIELD  (UINT8,    level             )
    FIELD  (UINT8,    vip               )
    FIELD  (UINT16,   exp               )
    FIELD_A(NCHAR,    name,         32  )
    FIELD  (STRING,   extra             )
    FIELD  (DOUBLE,   pay               )
TABLE_END(User)

TABLE_BEGIN(Server)
    FIELD  (UINT32,   id                )
    FIELD_A(NCHAR,    name,         32  )
    FIELD_A(NCHAR,    identifier,   32  )
    FIELD_A(NCHAR,    category,     16  )
TABLE_END(Server)

TABLE_BEGIN(Town)
    FIELD  (UINT32,   id                )
    FIELD  (UINT32,   user_id           )
    FIELD  (UINT16,   zone              )
    FIELD  (UINT16,   position          )
    FIELD  (UINT16,   level             )
    FIELD  (UINT16,   flags             )
    FIELD_A(NCHAR,    name,         32  )
    FIELD_A(NCHAR,    type,         16  )
    FIELD_A(NCHAR,    category,     16  )
TABLE_END(Town)

TABLE_BEGIN(Resource)
    FIELD  (UINT32,   id                )
    FIELD  (UINT32,   user_id           )
    FIELD  (UINT32,   town_id           )
    FIELD  (UINT64,   gold              )
    FIELD  (UINT64,   food              )
    FIELD  (UINT64,   wood              )
    FIELD  (UINT64,   stone             )
TABLE_END(Resource)

TABLE_BEGIN(Army)
    FIELD  (UINT32,   id                )
    FIELD  (UINT32,   user_id           )
    FIELD  (UINT32,   town_id           )
    FIELD  (UINT64,   army_1            )
    FIELD  (UINT64,   army_2            )
    FIELD  (UINT64,   army_3            )
TABLE_END(Army)

TABLE_BEGIN(Item)
    FIELD  (UINT32,   id                )
    FIELD  (UINT32,   user_id           )
    FIELD  (UINT32,   amount            )
    FIELD_A(NCHAR,    name,         32  )
TABLE_END(Item)

#pragma pack(1)

#endif
