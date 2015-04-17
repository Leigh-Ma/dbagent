/*Based on MySql 5.7 documentation*/

#include </usr/local/mysql/include/mysql.h>
#include "driver.h"

/* paramter check is done at higher lever */
static INT32 my_dr_init(DB_DR  *hdr, DB_CFG *cfg);
static void *my_dr_new_connector(DB_DR *hdr);
static INT32 my_co_connect(DB_CON *hdc);
static INT32 my_co_query(DB_CON* hdc, DB_REQ *req, DB_RESP *resp);
static INT32 my_co_update(DB_CON* hdc, DB_REQ *req, DB_RESP *resp);
static INT32 my_co_close(DB_CON  *hdc);


const static DB_OPR mysql_opr = {
        my_dr_init,
        my_dr_new_connector,
        my_co_connect,
        my_co_query,
        my_co_update,
        my_co_disconnect,
        my_co_close,
        my_co_tran_begin,
        my_co_tran_end,
        my_dr_destroy,
};

const DB_DR agt_mysql_driver = {"mysql", &mysql_opr, };


static INT32 my_dr_init(DB_DR  *hdr, DB_CFG *cfg) {

    hdr_lock(hdr);
    if(hdr->initailized == 0){      /* init only once */
        mysql_library_init(0, 0, 0);
        hdr->initailized = 1;
        memcpy(&hdr->cfg, cfg, sizeof(DB_CFG));
    }
    hdr_unlock(hdr);

    return 0;
}

static void *my_dr_new_connector(DB_DR *hdr) {
    MYSQL   *con = (MYSQL*)0;

    con = (MYSQL*)malloc(sizeof(MYSQL));
    if(con == (MYSQL*)0) {
        return (void*)0;
    }

    memset(con, 0x00, sizeof(MYSQL));
    con->reconnect = hdr->cfg->reconnect;                 /* automatic reconnect      */;

    mysql_thread_init();

    return con;
}

static INT32 my_co_connect(DB_CON *hdc) {
    DB_CFG *cfg;

    cfg = &hdc->driver->cfg;
    if(!mysql_real_connect(hdc->con, cfg->host, cfg->user, cfg->password,
            cfg->db, cfg->port, (const char *)0, 0)) {
        return -2;
    }
    return 0;
}

static INT32 my_co_query(DB_CON* hdc, DB_REQ *req, DB_RESP *resp) {
    INT32 row_num = 0, field_num, i, status;
    MYSQL_RES res;
    MYSQL_ROW row;

    if( 0 != mysql_query((MYSQL *)hdc->con, req->sql)) {
        logger("Execute Query error");
        return -1;
    }

    if((MYSQL_RES *)0 == (res = mysql_store_result((MYSQL *)hdc->con))) {
        logger("Store Query Result error");
        return -1;
    }

    iob_release(hdc->iob);
    while( (MYSQL_ROW)0 == (row = mysql_fetch_row((MYSQL *)hdc->con)) ){
        field_num = mysql_num_fields(res);
        status = iob_cache(hdc->iob, (void **)row, field_num, IOBF_CACHE_STRS, iob_vertical_cb);
        if(status != 0) {
            break;
        }
        row_num++;
    }
    mysql_free_result(res);

    return status;
}

static INT32 my_co_update(DB_CON* hdc, DB_REQ *req, DB_RESP *resp)  {
    UINT64 affected = 0;

    if( 0 != mysql_query((MYSQL *)hdc->con, req->sql)) {
        logger("Execute Update error");
        return -1;
    }

    if(affected == mysql_affected_rows(hdc->con)) {
        return -2;
    }
    resp->row_num = affected;

    return 0;
}

static INT32 my_co_disconnect(DB_CON *hdc) {

    mysql_close(hdc->con);
    return 0;
}

static INT32 my_co_close(DB_CON  *hdc) {

    mysql_thread_end();
    free(hdc->con);

    return 0;
}

static INT32 my_dr_destroy(DB_DR  *hdr) {
    INT32 status = 0;

    hdr_lock(hdr);
    if(hdr->initailized == 1) {
        if( && hdr->links == 0) {
            hdr->initailized = 0;
            mysql_library_end();
        } else {
            status = -2;
        }
    }
    hdr_unlock(hdr);

    return status;
}

static INT32 my_co_tran_begin(DB_CON *hdc) {
    INT32 status = 0;

    if( 0 != mysql_query((MYSQL *)hdc->con, "begin;")) {
        logger("Begin Transaction error");
        return -1;
    }

    return status;
}

static INT32 my_co_tran_end(DB_CON *hdc) {
    INT32 status = 0;

    if( 0 != mysql_query((MYSQL *)hdc->con, "commit;")) {
        logger("Begin Transaction error");
        return -1;
    }

    return status;
}


