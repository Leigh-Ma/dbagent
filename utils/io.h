#ifndef _DRIVER_IO_H_
#define _DRIVER_IO_H_
#include "../pub/pub.h"
#include <pthread.h>
#include <sys/uio.h>

#define IOV_BLOCK_SIZE      4096

/* all data is continuous as one message   */
#define IOBF_CACHE_NEXT     0x10000000      /*
                                             * write data to the next iovec, not used yet
                                             * if not set(default), append to current iovec
                                             */
#define IOBF_CACHE_FULL     0x40000000      /* DIBO is full                                             */
#define IOBF_CACHE_TAIL     0x80000000      /* DIBO is at last buff                                     */

#define IOBF_CACHE_STR      0x00000001      /* data is string form                                      */
#define IOBF_CACHE_STRS     0x00000002      /* data is a char*[]                                        */
#define IOBF_CACHE_BIN      0x00000004      /* data is in binary form                                   */


/* a continuous memory buff allocated at base,
 * split into @iov_len blocks of memory of same size IOV_SIZE
 * iovs and base are allocated together, and only iovs should be freed
 */
typedef struct driver_io_buffer {
    UINT32             flag;
    INT32              size;           /* read only: base buff size    */
    INT32              iov_len;        /* iov buff size                */
    INT32              iov_num;        /* iov num                      */

    pthread_mutex_t    lock;
    INT32              iov_index;      /* current processing iov index */
    INT32              iov_offset;     /* current offset in iov        */

    struct iovec       *iovs;          /* use able iovec array         */
    char               *base;          /* base address of buff         */

}DIOB, *HDIOB;


/* callback for each slice of data, return buffer offset by callback   */
typedef INT32 (*iob_wslice_cb)(char *buff, void *params, INT32 size, INT32 remain_size);

typedef struct driver_iob_cb{
    iob_wslice_cb   iob_cb;
    void            *param;
    INT32           param_size;
}DIOB_CB;

extern DIOB_CB iob_vertical_cb;

DIOB *iob_alloc_p(INT32 block, INT32 size);
DIOB *iob_alloc(INT32 block);
DIOB *iob_copy(DIOB *iob);
DIOB *iob_release(DIOB* iob);
void  iob_destroy(DIOB* iob);
INT32 iob_cache(DIOB *iob, void *data, INT32 size, UINT32 flag, DIOB_CB *callback);
INT32 iob_probe(DIOB *iob);

#endif
