/*Based on MySql 5.7 documentation*/

#include </usr/local/mysql/include/mysql.h>
#include "db.h"
#include "agt_db.h"
/*all apis should be thread safe*/

int g_driver_initialized = 0;

int db_init(HDB *hdb) {
	if(!g_driver_initialized){
		mysql_init((MYSQL *)0);
	}
	mysql_thread_init();

	return 0;
}

int db_connect(HDB *hdb, HUSER *hu) {

	MYSQL *con = (MYSQL*)malloc(sizeof(MYSQL));

	if(!con) {
		return -1;
	}

	hdb->con = mysql_real_connect(con, hu->host, hu->user, hu->password,
			hu->dbname, hu->port, (const char *)0, 0);
	if(!hdb->con) {
		logger("Connect to MySql database error! [host]%s:%d--[db]%s--[user]%s",
				hu->host, hu->port, hu->dbname, hu->user);
		free(con);
		return -1;
	}
	return 0;
}

int db_query(HDB* hdb, HQUERY *hq, HRESULT *hr, HTABLE *ht) {
	MYSQL_RES *res;
	MYSQL_ROW row;
	int row_num = 0, field_num, i;

	if( 0 != mysql_query((MYSQL *)hdb->con, hq->sql)) {
		logger("Execute Query error");
		return -1;
	}

	if((MYSQL_RES *)0 == (res = mysql_store_result((MYSQL *)hdb->con))) {
		logger("Store Query Result error");
		return -1;
	}

	hdb_close_cache(hdb);
	while(row = mysql_fetch_row((MYSQL *)hdb->con)){
		field_num = mysql_num_fields(res);
	    if(hdb_cache_mysql_row(hdb, row, field_num)) {
	    	hdb_close_cache(hdb);
	    	break;
	    }
		row_num++;
	}
	mysql_free_result(res);

	/*if row_num == 0 , means there is no result for this query*/
	return hdb_send_cache(hdb, HDB_IOV_LAST_PACKET | HDB_IOV_HEADER);;
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


