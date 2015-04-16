/*Based on MySql 5.7 documentation*/

#include </usr/local/mysql/include/mysql.h>
#include "driver.h"

/*all apis should be thread safe*/
const static DB_OPR mysql_opr = {
        db_init,
        db_new_connector,
        db_connect,
        db_query,
        db_update,
        db_close
};

const DB_DR agt_mysql_driver = {
        "mysql",
        &mysql_opr,
};


static INT32 db_init(DB_DR  *hdr, DB_CFG *cfg) {
    if(hdr == (DB_DR*)0 || cfg == (DB_CFG*)0) {
        return -1;
    }

    hdr_lock(hdr);
	if(hdr->initailized == 0){
		mysql_init((MYSQL *)0);
		hdr->initailized = 1;
	}
	memcpy(&hdr->cfg, cfg, sizeof(DB_CFG));
	hdr_unlock(hdr);

	mysql_thread_init();

	return 0;
}

static DB_CON *db_new_connector(DB_DR *hdr) {
    DB_CON  *hdc = (DB_CON*)0;

    if(hdr == (DB_DR*)0) {
        return (DB_CON*)0;
    }

    hdc = (DB_CON*)malloc(sizeof(DB_CON));
    if(hdc == (DB_CON*)0) {
        return (DB_CON*)0;
    }


    memset(hdc, 0x00, sizeof(DB_CON));
    hdc->iob = iob_alloc(8);
    if(hdc->iob == (DIOB *)0) {
        free(hdc);
        return (DB_CON*)0;
    }

    hdc->driver  = hdr;
    hdc->tid = self_tid();

    hdr_lock(hdr);
    hdc->next = hdr->connections;
    hdr->links += 1;
    hdr->connections = hdc;
    hdr_unlock(hdr);

    return 0;
}

static INT32 db_connect(DB_CON *hdc) {
    DB_CFG *cfg;
    if(hdc == (DB_CON*)0) {
        return -1;
    }
    cfg = &hdc->driver->cfg;
	hdc->con = mysql_real_connect((MYSQL*)0, cfg->host, cfg->user, cfg->password,
			cfg->db, cfg->port, (const char *)0, 0);
	if(hdc->con == (MYSQL*)0) {
		return -2;
	}
	return 0;
}

static INT32 db_query(DB_CON *hdc, char *sql) {
	MYSQL_RES *res;
	MYSQL_ROW row;
	INT32 row_num = 0, field_num, i, status;

	if( 0 != mysql_query((MYSQL *)hdc->con, sql)) {
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

int db_update(HDB *hdb) {
    if(hdb->con){
        mysql_close((MYSQL *)hdb->con);
        free(hdb->con);
        hdb->con = 0;
    }
    mysql_thread_end();
    return 0;
}


int db_close(HDB *hdb) {
	if(hdb->con){
		mysql_close((MYSQL *)hdb->con);
		free(hdb->con);
		hdb->con = 0;
	}
	mysql_thread_end();
	return 0;
}


