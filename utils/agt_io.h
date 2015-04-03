#ifndef _AGT_IO_H_
#define _AGT_IO_H_

#define IOV_VEC_NUM
#define IOV_BUFF_SIZE

typedef struct tag_IOVS{
	struct iovec     *vecs;     /*pointer to a iovec array, all buff size is @size*/
	int              vec_size;
	int              vec_num;   /*@vecs num*/

	int              data_size;
	int              buff_size;

	struct tag_IOVS	 *next;
}IOVS;

typedef struct tag_AGTIO{
	IOVS             *iov;
	IOVS             *tail;
	void             *buff;

}AGTIO;
#endif
