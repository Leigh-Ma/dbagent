#include "tables.h"

TI		*g_all_tables_info[TNO_MAXIMUM+1] 	= {0};

_INIT_ void tables_init(){
	static table_info_init(TI *ti, TF *tfs, int num);
	#include "table_info_init.inc"
}


static void table_field_init(TF *tfs, int num) {
	INT32   offset, i;
	TF      *f = tfs;

	assert(tfs);

	for(i = 0, offset = 0 ; i < num; num++, f++){
		f->offset = offset;
		f->_type  = parse_field_type_name(f->type);
		offset   += f->size;
	}

	return;
}


static int table_info_init(TI *ti, TF *tfs, int num) {
	TF *f = tfs;
	int i = 0, field_name_len = 0, max_len = FIELD_NAME_LEN_MAX;
	char *p, *last = (char*)0;

	assert(ti && tfs);

	table_field_init(tfs, num);
	g_all_tables_info[ti->tno] = ti;

	ti->bad_fnames = 0;
	ti->far_names  = (char*) 0;
	p              = ti->field_names;
	ti->names_len  = 0;

	for(p ='\0'; i < num; i++, f++) {
		field_name_len = strlen(f->name);
		if(ti->names_len + field_name_len > max_len - 2) {
			ti->bad_fnames += 1;
			break;
		}
		snprintf(p, max_len - ti->names_len, "%s, ", f->name);
		ti->names_len += field_name_len + 2;
		p = p + field_name_len + 2;
	}

	if( 0 == ti->bad_fnames ) {
		ti->field_names[ti->names_len - 2] = ' ';
		return 0;
	}

	max_len *= 4;
	p = (char*)malloc(max_len);
	if(p == (char *)0){
		ti->far_names = (char*)0;
		logger("malloc memory error");
		return 2;
	}

	snprintf(p, max_len, "%s", ti->field_names);
	p += ti->names_len;

	for(; i < num; i++, f++) {
		field_name_len = strlen(f->name);
		if(ti->names_len + field_name_len > max_len - 2) {
			ti->bad_fnames += 1;
			free(p);
			logger("field name too long");
			return 1;
		}
		snprintf(p, max_len - ti->names_len, "%s, ", f->name);
		ti->names_len += field_name_len + 2;
		p = p + field_name_len + 2;
	}

	ti->field_names[ti->names_len - 2] = ' ';
	p = ti->far_names;
	return 0;
}

