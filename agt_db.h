#ifndef _AGT_DB_H_
#define _AGT_DB_H_


#define RETURN_NUMBER

#define HDB_IOV_SIZE          4096
#define HDB_IOV_RESERVED      16
#define HDB_IOV_NUM           12

#define HDB_IOV_HEADER        0x0001
#define HDB_IOV_LAST_PACKET   0x0002
#define HDB_IOV_FREE_MEM      0x0001

struct tag_DBCONNECTION{
	/*mysql/oralce/mssql/sybase ect.*/
	char           type[8];
	void           *con;
	int            buff_size;
	int            data_size;
	int            water_mark;
	char           seperator;

	struct tag_DBPACKET_HEADER {
		char last_packet; /*'Y'/'N'*/
	}header;
	/*iov[0] is used to for the header*/
	struct iovec   iov[HDB_IOV_NUM];
	int            iov_size[HDB_IOV_NUM];
	int            cur_iov;

};

struct tag_DBUSERIFNO{
	/*database information*/
	char           host[32];
	unsigned int   port;
	char           dbname[32];
    /*client information*/
	char           user[32];
	char           password[128];

	char           peer_ip[32];
	unsigned int   peer_port;
	char           peer_app[32];
};


/*always use malloc to allocate new query*/
struct tag_DBQUERY{
	/*@type: Query Type*/
	unsigned char  type;

	/*@rpc: Rpc way between client and agent*/
	unsigned char  rpc;

	/*@sql_len: sql string len*/
	unsigned short sql_len;

	/*@rows: query how many rows? */
	int            rows;

	/*@sql: sql string*/
	char           sql[1];
};

typedef struct tag_DBCONNECTION  HDB;
typedef struct tag_DBUSERIFNO    HUSER;
typedef struct tag_DBQUERY       HQUERY;
typedef struct tag_DBRESULT      HRESULT;
typedef struct tag_DBTABLE       HTABLE;

/*do initialize the database driver*/
int db_init(HDB *hdb);

/*do connect to the specified databases*/
int db_connect(HDB *hdb, HUSER *hu);

/*do a sql query on the specified connection*/
int db_query(HDB* hdb, HQUERY *hq, HRESULT *hr, HTABLE *ht);

/*do a none query operation on the specified connection*/
int db_update(HDB* hdb, HQUERY *hq, HRESULT *hr);

/*do close connection*/
int db_close(HDB *hdb);


static inline int hdb_confirm_cache(HDB *hdb, int size) {
	hdb->iov[hdb->cur_iov].iov_len += size;
	return 0;
}

static inline int hiov_confirm_cache(HDB *hdb, int size) {
	hdb->iov[hdb->cur_iov].iov_len += size;
	return 0;
}

static inline int hdb_close_cache(HDB *hdb) {
	return hiov_close(hdb, 0);
}

static inline int hiov_close_cache(HDB *hdb) {
	return hiov_close(hdb, 0);
}

static inline int hdb_send_cache(HDB *hdb, short flag) {
	return hiov_send_cache(hdb, flag);
}

#endif
