#ifndef _RELATIONS_COM_H_
#define _RELATIONS_COM_H_

typedef struct table_data_compress{
    UINT32                          table_no;
    UINT32                          flag_opr;
    UINT32                          flag_db;
    UINT32                          flag_rc;        /* leader flag                          */

    INT32                           record_num;     /* elements in list record              */
    INT32                           query_num;      /* number of rows want to query         */
    TI                              *ti;
    char                            *record;        /* char pointer can be easily operated  */
    struct table_data_compress      *next;          /* record list, do not use ring         */
    struct table_data_compress      *recycle;       /* leader only, @next recycle list      */
    struct table_data_compress      *has;           /* related data compress of certain     */
                                                    /* table of this record                 */
    struct table_data_compress      *has_next;      /* always point to TCOM group leader    */
                                                    /* is a link list                       */

    struct table_data_compress      *belong;        /* group leader:these records belong to */
    char                            *should_free;   /* the rows from database, should free  */
    char                            *condition;     /* query condition, valid for leader,   */
                                                    /* and valid after execute data load    */
}TCOM;

typedef TCOM                         RCOM;          /* single row: RCOM.record_num = 1      */

typedef struct leading_data_compress {
    char                            table_name[32]; /* leading data's table name            */
    RCOM                            *leader;        /* single: leader->record_num = 1       */
    struct leading_data_compress    *next;          /* maintain in list*/
}LCOM;

#define _set_tc_flag_attr(tc, flag)     1
#define _get_tc_flag_attr(tc, flag)     1
#define _clr_tc_flag_attr(tc, flag)     0

#define _set_tc_flag_error(tc, flag)    0
#define _get_tc_flag_error(tc, flag)    0
#define _clr_tc_flag_error(tc, flag)    0

#define _clr_tc_flags(tc)               0


typedef UINT8 (*table_com_satisfy)(TCOM *);
typedef void* (*table_com_proccess)(TCOM *);

TCOM *table_com_set_condition(TCOM* tc, const char *condition);
TCOM *table_com_init_by_ti(TI *ti) ;
TCOM *table_com_init_by_tname(const char *table_name) ;
TCOM *table_com_init_by_tno(const UINT32 table_no);
TCOM *table_com_load_data(TCOM* tc, const char *condition);
TCOM *table_com_release_data(TCOM* tc);
TCOM *table_com_reload_data(TCOM* tc, const char *condition);
TCOM *table_com_destroy(TCOM* tc);
RCOM *table_com_to_row_com(TCOM *tc);
TCOM *table_com_show_data(TCOM* tc);
TCOM *table_com_find(TCOM *tc, table_com_satisfy satisfy);

TCOM *row_com_show_data(TCOM* tc);
TCOM *row_com_insert_data(TCOM* tc);
TCOM *row_com_save_data(TCOM* tc);
TCOM *row_com_reload_data(TCOM* tc);
TCOM *row_com_find_or_create_has(TCOM* tc, const char *table_name);
TCOM *row_com_has_table_com(TCOM* tc, const char *table_name, const char *condition);
LCOM *leader_com_create_by_condition(char *table_name, const char *condition);


#endif
