#ifndef _TBALE_PUB_H_
#define _TBALE_PUB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "../pub/pub.h"

#define is_big_endian g_is_big_endian

#define TABLE_BEGIN(table)              typedef struct _table_##table {

#define TABLE_END(table)                }table;

#define FIELD_A(type, name, len)        type name[len];

#define FIELD(type, name)               type name;


typedef struct tag_FILED_DESC {
    char * const        name;           /* field name, static field             */
    INT32               size;           /* = @num * @base_size , static field   */
    UINT16              num;            /* element number of array field        */
    UINT16              base_size;      /* size of one element                  */
    INT32               type;           /* FT_XXX bits                          */

    const char *        _type;          /* string representation for @type      */
    INT32               offset;         /* offset of this field                 */
}TF;

#define FIELD_NAME_LEN_MAX              511
typedef struct tag_TABLE_INFO {
    const int           tno;            /* TNO_XXX, table id in application     */
    const int           row_size;
    TF                  *tfs;
    int                 tfn;
    const char          *name;
    const char          *name_ex;
    const char          *far_id;        /* string: tablename_id                 */

    char                field_names[FIELD_NAME_LEN_MAX + 1];
                                        /* string: id, level, vip, ...          */
    int                 bad_fnames;     /* @fnames length not long enough       */
    int                 names_len;      /* fnames or far_names string length    */
    char                *far_names;     /* @malloc memory for field name list   */
}TI;

#define TI_OF(table)                    g_all_tables_info[TNO_##table]
#define table_info_of(table)            g_all_tables_info[TNO_##table]

#define TI_OF_TNO(table)                g_all_tables_info[TNO_##table]
#define table_info_of_tno(tno)          g_all_tables_info[tno]

#include "tables.h"
#include "table_parse.h"
#include "table_query.h"
#include "table_info.h"
#include "relations.h"

extern const struct FT_NAME_MAP g_type_name_map[];
extern const int    g_field_type_num;
extern char    g_is_big_endian;
extern TI *g_all_tables_info[];

_INIT_ void tables_init();
void tables_show();
int  table_rows_show(const char *name, const void *rows, int num);
_TRY_ int table_rows_release(const char *name, void *rows, int num);

/* *********************************************************************************
 * function: parse_field_type_name
 * return the string name of the field type
 * *********************************************************************************/
char const *parse_field_type_name(INT32 type);

/* *********************************************************************************
 * function: parse_field_type_name
 * return the string name for sql of the field type
 * *********************************************************************************/
char const *parse_field_type_sql_name(INT32 type);


/* *********************************************************************************
 * function: parse_field_from_str
 * parse table field data from it's string form in @p into buffer @buff
 * *********************************************************************************/
int parse_field_from_str(const TF *f, const char *p, int len, char *buff);

/* *********************************************************************************
 * function: parse_row_from_str
 * parse table row data from it's string form in @p into buffer @buff
 * *********************************************************************************/
int parse_row_from_str(const char *row, const TF *tfs, int tfn, int len, char *record, char **tail);


const TI *query_get_table_info_by_name(const char *table_name);

const TI *query_get_table_info_by_tno(const int tno);

int find_rows_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num);

int find_rows_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num);

int find_all_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num);

int find_all_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num);

int find_with_cond_with_tno  (const int  tno,         const char *condition_with_where , _FREE_ void **rows, int *num);

int find_with_cond_with_tname(const char *table_name, const char *condition_with_where , _FREE_ void **rows, int *num);


#endif
