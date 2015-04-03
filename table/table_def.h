#ifndef _TBALE_DEF_H_
#define _TBALE_DEF_H_

#define TABLE_BEGIN(table)             \
	typedef struct _table_##table {

#define TABLE_END(table)               \
	}table;

#define FIELD_A(type, name, len)       \
	type name[len];

#define FIELD(type, name)              \
	type name;

typedef unsigned char (prase_val)(const void* buff, void* local, int* size);

typedef struct tag_FILED_DESC{
	CHAR 				*_type; /*type 字符串*/
	CHAR 				*name;  /**/
	INT32   			size;   /*size = num * base_size */
	UINT16				num;
	UINT16     			base_size;
	INT32               type;  /*FT_XXX bits*/
}TF;

unsigned char tf_prase(const void* buff, void* local, int* size);
#endif
