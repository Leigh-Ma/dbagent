#ifndef _TBALE_PUB_H_
#define _TBALE_PUB_H_
#include "../pub/pub.h"

#define TABLE_BEGIN(table) 				typedef struct _table_##table {

#define TABLE_END(table)				}table;

#define FIELD_A(type, name, len)		type name[len];

#define FIELD(type, name)				type name;


typedef struct tag_FILED_DESC {
	char * const		name;			/* field name, static field				*/
	INT32				size;			/* = @num * @base_size , static field 	*/
	UINT16				num;			/* element number of array field		*/
	UINT16				base_size;		/* size of one element					*/
	INT32				type;			/* FT_XXX bits							*/

	const char *		_type;			/* string representation for @type		*/
	INT32				offset;			/* offset of this field					*/
}TF;

#define FIELD_NAME_LEN_MAX				511
typedef struct tag_TABLE_INFO {
	const int			tno;			/*TNO_XXX, table id in application		*/
	const int           row_size;
	TF					*tfs;
	int					tfn;
	const char			*name;
	const char          *name_ex;

	char				field_names[FIELD_NAME_LEN_MAX + 1];
										/*string: id, level, vip, ...			*/
	int					bad_fnames;		/*@fnames length not long enough		*/
	int					names_len;		/*fnames or far_names string length		*/
	char				*far_names;		/*@malloc memory for field name list	*/
}TI;

#define TI_OF(table)					&g_all_tables_info[TNO_##table]
#define table_info_of(table)			&g_all_tables_info[TNO_##table]

#include "table_parse.h"
#include "table_query.h"
#include "table_info.h"

#endif
