#include "table_pub.h"

int row_save(TI *ti, void *row) {
    char   buff[ROW_SQL_BUFF_LEN] = {0};
    INT32  i, len = 0;  /* @i, @len, @buff are used in macros, do not modify */
    TF     *tf;
    void   *p;

    if(ti == (TI*)0 || row == (void*)0) {
        return -1;
    }

    len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "update %s set ", ti->name);
    for(i = 1, tf = &ti->tfs[1]; i< ti->tfn; i++, tf++) {
        p = (void *)((char*)row + tf->offset);
        _save_begin(tf)
            _save_try_string(NCHAR,  tf, p,  "%s")
            _save_try_far_string(STRING, tf, p,  "%s")
            _save_try(INT8,    tf, p, "%d")
            _save_try(INT16,   tf, p, "%d")
            _save_try(INT32,   tf, p, "%d")
            _save_try(INT64,   tf, p, "%lld")

            _save_try(FLOAT,   tf, p, "%f")
            _save_try(DOUBLE,  tf, p, "%lf")

            _save_try(CHAR,    tf, p,  "%c")

            _save_try(UINT8,   tf, p,  "%u")
            _save_try(UINT16,  tf, p,  "%u")
            _save_try(UINT32,  tf, p,  "%u")
            _save_try(UINT64,  tf, p,  "%llu")
        _save_end(tf)
    }
    len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "where id = %u\n", *(UINT32*)row);
    printf(buff);

    return 0;
}

int row_show(TI *ti, void *row) {
    char buff[ROW_SQL_BUFF_LEN] = {0};
    INT32  i, len = 0;  /* @i, @len, @buff are used in macros, do not modify */
    TF     *tf;
    void  *p;

    if(ti == (TI*)0 || row == (void*)0) {
        return -1;
    }

    len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "<%s: { ", ti->name_ex);
    for(i = 0, tf = ti->tfs; i < ti->tfn; tf++, i++) {
        p = (void *)((char*)row + tf->offset);
        _show_begin(tf)
            _show_try_string(NCHAR,  tf, p,  "%s")
            _show_try_far_string(STRING, tf, p,  "%s")
            _show_try(INT8,    tf, p, "%d")
            _show_try(INT16,   tf, p, "%d")
            _show_try(INT32,   tf, p, "%d")
            _show_try(INT64,   tf, p, "%lld")

            _show_try(FLOAT,   tf, p, "%f")
            _show_try(DOUBLE,  tf, p, "%lf")

            _show_try(CHAR,    tf, p,  "%c")

            _show_try(UINT8,   tf, p,  "%u")
            _show_try(UINT16,  tf, p,  "%u")
            _show_try(UINT32,  tf, p,  "%u")
            _show_try(UINT64,  tf, p,  "%llu")
        _show_end(tf)
    }
    snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "} >\n");
    printf(buff);

    return 0;
}

int row_insert(TI *ti, void *row) {
    char   buff[ROW_SQL_BUFF_LEN] = {0};
    INT32  i, len = 0;  /* @i, @len, @buff are used in macros, do not modify */
    TF     *tf;
    void   *p;

    if(ti == (TI*)0 || row == (void*)0) {
        return -1;
    }

    if(ti->bad_fnames) {
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "insert into %s(%s) values (", ti->name, ti->far_names);
    } else {
        len += snprintf(buff + len, ROW_SQL_BUFF_LEN - len, "insert into %s(%s) values (", ti->name, ti->field_names);
    }
    for(i = 0, tf = ti->tfs; i < ti->tfn; tf++, i++) {
        p = (void *)((char*)row + tf->offset);
        _insert_begin(tf)
            _insert_try_string(NCHAR,  tf, p,  "%s")
            _insert_try_far_string(STRING, tf, p,  "%s")
            _insert_try(INT8,    tf, p, "%d")
            _insert_try(INT16,   tf, p, "%d")
            _insert_try(INT32,   tf, p, "%d")
            _insert_try(INT64,   tf, p, "%lld")

            _insert_try(FLOAT,   tf, p, "%f")
            _insert_try(DOUBLE,  tf, p, "%lf")

            _insert_try(CHAR,    tf, p,  "%c")

            _insert_try(UINT8,   tf, p,  "%u")
            _insert_try(UINT16,  tf, p,  "%u")
            _insert_try(UINT32,  tf, p,  "%u")
            _insert_try(UINT64,  tf, p,  "%llu")
        _insert_end(tf)
    }
    snprintf(buff + len, ROW_SQL_BUFF_LEN - len, ")\n");
    printf(buff);

    return 0;
}


