#include "table_pub.h"

TI      *g_all_tables_info[TNO_MAXIMUM+1]   = {0};
char    g_is_big_endian = 0;

static int table_info_init(TI *ti, TF *tfs, int num);

_INIT_ void tables_init(){
     short _short = 0x0102;
     char *_byte  = (char*)&_short;
     g_is_big_endian =  (*_byte == 0x02);

    #include "table_info_init.inc"
}

_TRY_ int table_rows_release_by_name(const char *name, void *rows, int num) {
    char *p = (char*)rows, *q = (char*)rows;
    int i, j;
    TI *ti;
    TF *tf;

    _CHECK_RET(name && rows, PR_ERR_PARAM);

    _get_ti_by_tname(name, ti);

    for(j = 0, tf = ti->tfs; j < ti->tfn; j++, tf++) {
        if(FT_SHOULD_FREE(tf->type)) {
            for(i = 0 ;i < num; i++) {
                p = q + i * ti->row_size + tf->offset;
                if(*(char**)p) {
                    free(*(char**)p);
                }
            }
        }
    }
    free(rows);
    return PR_OK;
}

_TRY_ int table_rows_release_by_ti(TI *ti, void *rows, int num) {
    char *p = (char*)rows, *q = (char*)rows;
    int i, j;
    TF *tf;

    _CHECK_RET(ti && rows, PR_ERR_PARAM);

    for(j = 0, tf = ti->tfs; j < ti->tfn; j++, tf++) {
        if(FT_SHOULD_FREE(tf->type)) {
            for(i = 0 ;i < num; i++) {
                p = q + i * ti->row_size + tf->offset;
                if(*(char**)p) {
                    free(*(char**)p);
                }
            }
        }
    }
    free(rows);
    return PR_OK;
}

void tables_show() {
    int i = TNO_MINIMUM;
    for(; i <= TNO_MAXIMUM; i++) {
        table_info_show(table_info_of_tno(i));
    }
}


int table_rows_show(const char *name, const void *rows, int num) {
    int i;
    TI *ti;

    _CHECK_RET(name && rows, PR_ERR_PARAM);

    _get_ti_by_tname(name, ti);

    for(i = 0; i< num; i++) {
        row_show(ti, (void*)((char*)rows + i*ti->row_size));
    }

    return PR_OK;
}

void table_info_show(TI *ti) {
    TF *tf = (TI *)0;
    int i = 1;

    if(!ti) {
        return;
    }

    printf("/* %s:\t table no (%3d), row size %4d, %3d fields: (%s) [%4d chars] */\n", ti->name_ex,
            ti->tno, ti->row_size, ti->tfn ,ti->field_names, ti->names_len);
    printf("create table %s (\n", ti->name);
    for(tf = ti->tfs; i <= ti->tfn; i++, tf++) {
        if(tf->num > 1) {
            printf("\t %16s\t %s(%d), \t/* %3d (base size %2d), offset %4d, type 0x%08x */\n", tf->name,
                    parse_field_type_sql_name(tf->type), tf->num,
                    tf->size, tf->base_size, tf->offset, tf->type);
        } else {
            printf("\t %16s\t %-8s, \t/* %3d (base size %2d), offset %4d, type 0x%08x */\n", tf->name,
                    parse_field_type_sql_name(tf->type),
                    tf->size, tf->base_size, tf->offset, tf->type);
        }
    }
    printf("); /* @%s */\n\n", ti->name_ex);
}

static void table_field_init(TF *tfs, int num) {
    INT32   offset, i;
    TF      *f = tfs;

    assert(tfs);

    for(i = 0, offset = 0 ; i < num; i++, f++){
        f->offset = offset;
        f->_type  = parse_field_type_name(f->type);
        offset   += f->size;
    }

    return;
}

static int table_info_init(TI *ti, TF *tfs, int num) {
    TF *f = tfs;
    int i = 0, field_name_len = 0, max_len = FIELD_NAME_LEN_MAX;
    char *p;

    assert(ti && tfs);

    table_field_init(tfs, num);
    g_all_tables_info[ti->tno] = ti;

    ti->bad_fnames = 0;
    ti->far_names  = (char*) 0;
    p              = ti->field_names;
    ti->names_len  = 0;

    for(*p ='\0'; i < num; i++, f++) {
        field_name_len = strlen(f->name);
        if(ti->names_len + field_name_len > max_len - 4) {
            ti->bad_fnames += 1;
            break;
        }
        snprintf(p, max_len - ti->names_len, "`%s`, ", f->name);
        ti->names_len += field_name_len + 4;
        p = p + field_name_len + 4;
    }

    if( 0 == ti->bad_fnames ) {
        ti->field_names[ti->names_len - 2] = ' ';
        ti->field_names[ti->names_len - 1] = '\0';
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



