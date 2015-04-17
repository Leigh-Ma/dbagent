#ifndef _DRIVER_H_
#define _DRIVER_H_
#include "utils/io.h"

#define db_lock_t           pthread_mutex_t
#define db_thread_t         pthread_t
#define dr_lock(hdr)       pthread_mutex_lock(&((hdr)->lock))
#define dr_unlock(hdr)     pthread_mutex_unlock(&((hdr)->lock))

typedef struct db_connection {
    void                        *con;
    struct db_driver            *driver;
    db_thread_t                 tid;
    DIOB                        *iob;
    INT32                       status;         /*0: disconnected, 1: connected */
    struct db_operates          *opr;

    struct db_connection        *next;

}DB_CON;

typedef struct db_server_config {
    UINT32                      port;            /* database server listening port           */
    char                        host[64];        /* database server host                     */

    char                        db[32];          /* database name                            */
    char                        user[32];        /* login user                               */
    char                        password[128];   /* password                                 */
    INT32                       reconnect;
}DB_CFG;

typedef struct db_operate_respond{
    DIOB                        *iob;
    INT32                       row_num;
}DB_RESP;

typedef struct db_operate_request{
    char                        *sql;
}DB_REQ;

typedef struct db_driver{
    char                        *type;           /* mysql/oralce/mssql/sybase ect.           */
    const struct db_operates* const opr;

    char                        *info;           /* description for driver                   */

    DB_CFG                      cfg;
    INT32                       initailized;
    INT32                       links;           /**/
    INT32                       linked;          /**/

    db_lock_t                   lock;
    DB_CON                      *connections;    /* lower level database connection handle   */
}DB_DR;


/*do initialize the database driver             */
typedef INT32 (dr_init_t)(DB_DR *hdr, DB_CFG *cfg);

/*do initialize a new connection                */
typedef void* (dr_new_t)(DB_DR *hdr);

/*do connect to the specified databases         */
typedef INT32 (co_connect_t)(DB_CON *hdc);

/*do disconnect from the specified databases    */
typedef INT32 (co_disconnect_t)(DB_CON *hdc);

/*do a sql query on the specified connection    */
typedef INT32 (co_query_t)(DB_CON* hdc, DB_REQ *req, DB_RESP *res);

/*do a none query operation on the specified connection*/
typedef INT32 (co_update_t)(DB_CON* hdc, DB_REQ *req, DB_RESP *res);

/*do close connection                           */
typedef INT32 (co_close_t)(DB_CON *db_con);

/*begin a transaction on the connection         */
typedef INT32 (co_tran_begin_t)(DB_CON *hdc);

/*end a transaction on the connection         */
typedef INT32 (co_tran_end_t)(DB_CON *hdc);

/*end using driver                              */
typedef INT32 (dr_end_t)(DB_DR *hdr);


typedef struct db_operates {
    dr_init_t           *dr_init;
    dr_new_t            *dr_connector;
    co_connect_t        *co_connect;
    co_query_t          *co_query;
    co_update_t         *co_update;
    co_disconnect_t     *co_diconnect;
    co_close_t          *co_close;
    co_tran_begin_t     *co_tran_begin;
    co_tran_end_t       *co_tran_end;
    dr_end_t            *dr_end;

}DB_OPR;


#endif
