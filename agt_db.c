#include "agt_db.h"

/*hdb iovec manage apis*/

static int hiov_alloc(HDB *hdb) {
	struct iovec *next;

	if(1 + hdb->cur_iov > HDB_IOV_NUM) {
		if(hdb_send_cache(hdb, 0)) {
			logger("Iovec in HDB is full, send cache error");
			return -1;
		}
	}

	next = &(hdb->iov[++hdb->cur_iov]);
	if((void*)0 == next->iov_base) {
		next->iov_base = malloc(HDB_IOV_SIZE);
		if((void*)0 == next->iov_base) {
			next->iov_len = 0;
			logger("Iovec malloc memory error");
			return -1;
		}
		hdb->iov_size[hdb->cur_iov] = HDB_IOV_SIZE;
	}
	next->iov_len = 0;

	return 0;
}

static int hiov_set_first(HDB *hdb, void *data, size_t size){
	hdb->iov[0].iov_base = data;
	hdb->iov[0].iov_len  = size;
	return 0;
}

int hiov_send_cache(HDB *hdb, short flag) {
	int ret,i;

	hdb->header.last_packet = (flag & HDB_IOV_LAST_PACKET) ? 'N' : 'Y';
	if(flag & HDB_IOV_HEADER) {
		hiov_set_first(hdb, &hdb->header, sizeof(hdb->header));
	}
	ret = hdb_send_iovec(hdb->iov);
	hiov_close(hdb, 0);

	return ret;
}

static int hiov_close(HDB *hdb, short flag) {
	int i;

	assert(hdb);

	for(i = 0; i< HDB_IOV_NUM; i++) {
		hdb->iov[i].iov_len = 0;
		if(flag & HDB_IOV_FREE_MEM) {
			if(hdb->iov[i].iov_base) {
				free(hdb->iov[i].iov_base);
				hdb->iov[i].iov_base = 0;
			}
		}
	}
	hdb->cur_iov = 0;

	return 0;
}

static int RETURN_NUMBER hiov_cur_free(HDB *hdb) {
	int index;

	assert(hdb != (HDB*)0);

	if(hdb->cur_iov == 0) {
		hiov_alloc(hdb);
	}
	index = hdb->cur_iov;

	return hdb->iov_size[index] - hdb->iov[index].iov_len;
}

static char *hiov_cur_buff(HDB *hdb){
	struct iovec *cur;

	assert(hdb != (HDB*)0);

	if(hdb->cur_iov == 0) {
		hiov_alloc(hdb);
	}

	cur = &(hdb->iov[hdb->cur_iov]);

	return (( char*)cur->iov_base) + hdb->iov_size[hdb->cur_iov];
}


int hdb_cache_mysql_row(HDB *hdb, char **fields, int num) {
	int  i = 0, head_wrote = 0, cached_len, field_len, buff_len;
	char *buff, row_head[HDB_IOV_RESERVED]={0};

    snprintf(row_head, HDB_IOV_RESERVED, '%d', num);
next:
    cached_len = 0; /*cached*/
    buff     = hiov_cur_buff(hdb);
    buff_len = hiov_cur_free(hdb);

	for(; i < num; i++){
		if(head_wrote==0) {
		    if((cached_len = strlen(row_head)) > buff_len - 1) {
		    	cached_len = 0;
		    	break;
		    }
		    snprintf(buff, buff_len, "%s%c", row_head, hdb->seperator);
		    head_wrote = 1;
		}
		field_len = strlen(fields[i]);
		if(cached_len + field_len > buff_len - 1) {
			break;
		}
		snprintf(buff+cached_len, field_len, "%s%c", fields[i], hdb->seperator);
		cached_len += field_len + 1;
	}
	hiov_confirm_cache(hdb, cached_len);
	if(i < num){
		if(hiov_alloc(hdb)) {
			return -1;
		}
		goto next;
	}

	return 0;
}

int hdb_init(HDB *hdb) {
	assert(hdb);
	memset(hdb, 0x00, sizeof(HDB));
	return 0;
}

int hdb_close(HDB *hdb) {
	hiov_close(hdb, HDB_IOV_FREE_MEM);
	return 0;
}
