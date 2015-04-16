#ifndef _DRIVER_H_
#define _DRIVER_H_
#include "utils/io.h"

#define db_lock_t           pthread_mutex_t
#define db_thread_t         pthread_t
#define hdr_lock(hdr)       pthread_mutex_lock(&((hdr)->lock))
#define hdr_unlock(hdr)     pthread_mutex_unlock(&((hdr)->lock))

typedef struct db_connection {
    void                        *con;
    struct db_driver            *driver;
    db_thread_t                 tid;
    DIOB                        *iob;
    INT32                       status;
    struct db_operates          *opr;

    struct db_connection        *next;

}DB_CON;

typedef struct db_server_config {
    UINT32                      port;            /* database server listening port           */
    char                        host[64];        /* database server host                     */

    char                        db[32];          /* database name                            */
    char                        user[32];        /* login user                               */
    char                        password[128];   /* password                                 */
}DB_CFG;

typedef struct db_driver{
    char                        *type;           /* mysql/oralce/mssql/sybase ect.           */
    const struct db_operates* const opr;
    DB_CFG                      cfg;
    INT32                       initailized;
    INT32                       links;

    db_lock_t                   lock;
    DB_CON                      *connections;    /* lower level database connection handle   */
 }DB_DR;


/*do initialize the database driver             */
typedef INT32 (db_init_t)(DB_DR *hdr, DB_CFG *cfg);

/*do initialize a new connection                */
typedef DB_CON (db_new_connector_t)(DB_DR *hdr);

/*do connect to the specified databases         */
typedef INT32 (db_connect_t)(DB_CON *hdc);

/*do a sql query on the specified connection    */
typedef INT32 (db_query_t)(DB_CON* hdc);

/*do a none query operation on the specified connection*/
typedef INT32 (db_update_t)(DB_CON* hdc);

/*do close connection                           */
typedef INT32 (db_close_t)(DB_CON *hdc);


typedef struct db_operates {
    db_init_t           *init;
    db_new_connector_t  *new_connect;
    db_connect_t        *connect;
    db_query_t          *query;
    db_update_t         *update;
    db_close_t          *close;
}DB_OPR;


#endif
