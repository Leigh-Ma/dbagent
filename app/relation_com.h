#ifndef _RELATION_COM_H_
#define _RELATION_COM_H_

typedef struct table_data_compress{
    UINT32                          table_no;
    UINT32                          flag_opr;
    UINT32                          flag_db;
    UINT32                          flag_rc;        /* leader flag                          */

    INT32                           record_num;     /* elements in list record              */
    INT32                           query_num;      /* number of rows want to query         */
    TI                              *ti;
    char                            *record;        /* char pointer can be easily operated  */
    TCOM                            *next;          /* record ring                          */
    TCOM                            *recycle;       /* leader only, @next recycle list      */
    TCOM                            *has;           /* related data of this record          */
    TCOM                            *belong;        /* this record belongs to               */
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

#define _set_tc_flag(tc, flag)
#define _get_tc_flag(tc, flag)
#define _clr_tc_flag(tc, flag)


#endif
