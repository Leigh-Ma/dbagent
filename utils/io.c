#include "io.h"

DIOB *iob_alloc_b(INT32 block, INT32 size) {
    DIOB *iob = (DIOB*)0;
    INT32 i = 0;

    if(block <= 0 || size <= 0) {
        return iob;
    }

    iob = (DIOB*)malloc(sizeof(DIOB));
    if(iob == (DIOB*)0) {
        return iob;
    }

    memset(iob, 0x00, sizeof(DIOB));
    iob->iovs = (struct iovec *)malloc(block*(size + sizeof(struct iovec)));
    if(iob->iovs == (struct iovec *)0) {
        free(iob);
        return (DIOB*)0;
    }

    iob->base = (char*)iob->iovs + sizeof(struct iovec) * block;
    iob->size = block * size;
    iob->iov_num = block;
    iob->iov_len = size;

    for(i = 0; i < block; i++) {
        iob->iovs[i].iov_base = (void*)(iob->base + block * i);
    }

    return iob;
}

DIOB *iob_alloc(INT32 block) {
    return iob_alloc_b(block, IOV_BLOCK_SIZE);
}


DIOB *iob_copy(DIOB *iob) {
    DIOB *cp = (DIOB*)0;
    INT32 new_size, new_num;
    char *new_base;

    if(iob == (DIOB*)0 || iob->iov_num == 0) {
       return (DIOB*)0;
    }

    cp = (DIOB *)malloc(sizeof(DIOB));
    if(cp == (DIOB*)0) {
       return (DIOB*)0;
    }

    new_num = iob->iov_index;
    if(iob->iov_offset != 0) {
        new_num += 1;
    }
    new_size = new_num * (sizeof(struct iovec) + iob->iov_len );
    new_base =(char*)malloc(new_size);
    if(new_base == (char*)0) {
        free(cp);
        return (DIOB *)0;
    }

    memset(cp, 0x00, sizeof(DIOB));
    memset(new_base, 0x00, new_size);

    memcpy(new_base, iob->iovs, sizeof(struct iovec) * new_num);
    memcpy(new_base + new_num * sizeof(struct iovec), iob->base, new_num * iob->iov_len);

    cp->flag        = iob->flag;
    cp->iov_num     = new_num;
    cp->iov_index   = iob->iov_index;
    cp->iov_len     = iob->iov_len;
    cp->iov_offset  = iob->iov_offset;

    cp->iovs = (struct iovec *)new_base;
    cp->base = new_base + new_num * sizeof(struct iovec);
    cp->size = new_num * iob->iov_len;
    for(new_num = 0; new_num < cp->iov_num; new_num++) {
        cp->iovs[new_num].iov_base = cp->base + cp->iov_len * new_num;
    }

    return cp;
}

DIOB *iob_release(DIOB* iob) {
    INT32 i = 0;

    if(iob == (DIOB*)0) {
        return iob;
    }

    memset(iob->base, 0x00, iob->size);
    for(i = 0; i < iob->iov_num; i++) {
        iob->iovs[i].iov_len = 0;
    }
    iob->flag       = 0;
    iob->iov_index  = 0;
    iob->iov_offset = 0;

    return iob;
}


void iob_destroy(DIOB* iob) {
    if(iob != (DIOB*)0) {
        free(iob->iovs);
        free(iob);
    }
}

static INT32 iob_enlarge(DIOB *iob, INT32 bytes) {
    INT32 new_size, new_num;
    char *new_base;

    if(iob == (DIOB*)0 || iob->iov_num == 0) {
        return ERR_PARAM;
    }
    if(bytes <= 0) {
        new_num = 2 * iob->iov_num;
    } else {
        new_num = iob->iov_num + 4 * (((IOV_BLOCK_SIZE + bytes)/IOV_BLOCK_SIZE + 4)/4);
    }
    new_size = new_num * (sizeof(struct iovec) + iob->iov_len );
    new_base = (char*)malloc(new_size);
    if(new_base == (char*)0) {
        return ERR_SYS_MEM;
    }
    memset(new_base, 0x00, new_size);
    memcpy(new_base, iob->iovs, sizeof(struct iovec) * iob->iov_num);
    memcpy(new_base + new_num * sizeof(struct iovec), iob->base, iob->size);

    free(iob->iovs);

    iob->iovs = (struct iovec *)new_base;
    iob->base = new_base + new_num * sizeof(struct iovec);

    for(iob->iov_num = 0;iob->iov_num < new_num; iob->iov_num++) {
        iob->iovs[iob->iov_num].iov_base = (void*)(iob->base + iob->iov_num * iob->iov_len);
    }
    iob->size = new_num * iob->iov_len;

    return 0;
}

/*
 * size <  0 for block type string
 * size >= 0 for block type bin
 */
static INT32 iob_cache_block(DIOB *iob, void *block, INT32 size, UINT32 flag, DIOB_CB *cb) {
    INT32 available = 0, offset, start, iov_offset;

    if(block == (void*)0) {
        size = 0;
    } else if (size < 0) {
        size = strlen((char*)block);
    }

    start = iob->iov_index;

    iov_offset = iob->iov_offset;
    if((flag & IOBF_CACHE_NEXT ) && iob->iov_offset != 0) {
        start = iob->iov_index + 1;
        iov_offset = 0;
    }

    offset    = start * iob->iov_len + iov_offset;
    available = iob->size - offset;

    if(cb && cb->iob_cb) {
        if(size + cb->param_size >= available) {
            if(iob_enlarge(iob, 1 + size + cb->param_size - available)) {
                return ERR_SYS_MEM;
            }
        }
        offset += cb->iob_cb(iob->base + offset, cb->param, cb->param_size, 0);
    } else {
        if (size >= available) {
           if(iob_enlarge(iob, 1 + size - available)) {
               return ERR_SYS_MEM;
           }
        }
    }

    iob->iov_offset = iov_offset;

    if(size > 0) {
        memcpy(iob->base + offset, block, size);
        offset += size;
    }

    iob->iov_index  = offset / iob->iov_len;
    iob->iov_offset = offset % iob->iov_len;

    for(; start < iob->iov_index; start++) {
        iob->iovs[start].iov_len = iob->iov_len;
    }
    iob->iovs[start].iov_len = iob->iov_offset;

    return 0;
}

static INT32 iob_cache_strings(DIOB *iob, char **str, INT32 size, UINT32 flag, DIOB_CB *cb) {
    INT32 available = 0, cb_prepend = 0, len;
    INT32 offset, iov_offset, i, start, *str_lens;
    char *buff;

    if(size <= 0) {
        return ERR_PARAM;
    }

    str_lens = malloc(size * sizeof(INT32));
    if(str_lens == (INT32*)0) {
        return ERR_SYS_MEM;
    }

    if(cb && cb->iob_cb) {
        cb_prepend = 1;         /* should call cb->iob_cb*/
        len += size * cb_prepend;
    }

    for(i = 0, len = 0; i < size; i++) {
        if(str[i]) {
            str_lens[i] = strlen(str[i]);
            len += str_lens[i];
        } else {
            str_lens[i] = 0;
        }
    }

    start = iob->iov_index;
    iov_offset = iob->iov_offset;
    if( (flag & IOBF_CACHE_NEXT) && (iob->iov_offset != 0) ) {
        start = iob->iov_index + 1;
        iov_offset = 0;
    }

    offset    = start * iob->iov_len + iov_offset;
    available = iob->size - offset;

    if(len >= available) {
        if(iob_enlarge(iob, len - available)) {
            return ERR_SYS_MEM;
        }
    }

    iob->iov_offset = iov_offset;
    buff = iob->base + offset;

    for(i = 0, len = 0; i < size; i++) {
        if(cb_prepend > 0) {
            buff += cb->iob_cb(buff, cb->param, cb->param_size, 0);
        }
        if(str_lens[i] > 0) {
            memcpy(buff, str[i], str_lens[i]);
            buff += str_lens[i];
        }
    }

    offset = buff - iob->base;
    iob->iov_index  = offset / iob->iov_len;
    iob->iov_offset = offset % iob->iov_len;
    for(; start < iob->iov_index; start++) {
        iob->iovs[start].iov_len = iob->iov_len;
    }
    iob->iovs[start].iov_len = iob->iov_offset;

    return 0;
}

INT32 iob_cache(DIOB *iob, void *data, INT32 size, UINT32 flag, DIOB_CB *callback) {
    INT32 status = ERR_IOB_FLAG;

    if(iob == (DIOB*)0) {
        return ERR_PARAM;
    }
    if(iob->flag & IOBF_CACHE_FULL) { /* not useful when enlarge ability provided */
        return ERR_IOB_FULL;
    }

    if(flag & IOBF_CACHE_BIN) {
        status = iob_cache_block(iob, data, size, flag, callback);
    }else if(flag & IOBF_CACHE_STR) {
        status = iob_cache_block(iob, data, size, flag, callback);
    }else if (flag & IOBF_CACHE_STRS) {
        status = iob_cache_strings(iob, (char**) data, size, flag, callback);
    }

    return status;
}


INT32 iob_proccess(DIOB *iob, DIOB_CB *cb) {
    INT32 len = 0, shift = ERR_IOB_EMPTY;
    INT32 last_index = -1, offset, index, remain;
    char  *buff;

    _CHECK_PARAMS_RET(
       iob != (DIOB*)0 && cb != (DIOB_CB*)0,
       ERR_PARAM
    )

    do {
        index  = len / iob->iov_len;
        offset = len % iob->iov_len;
        do {
            if(index > iob->iov_index) {
                return 0;
            }
            if(offset >= iob->iovs[index].iov_len) {
                index += 1;
                offset = 0;
            } else {
                break;
            }
        } while(1);
        if(index >= iob->iov_num) {
            return 0;
        }
        if(last_index != index) {
            printf("\n processing block %d at %p----(%4d/%d)------\n",
                    index, iob->iovs[index].iov_base, (int32_t)iob->iovs[index].iov_len, iob->iov_len);
        }

        last_index = index;
        buff   = iob->base + index * iob->iov_len + offset;
        remain = iob->iov_index * iob->iov_len + iob->iov_offset - (buff - iob->base);
        shift  = cb->iob_cb(buff, cb->param, cb->param_size, remain);
        len   += shift;
    } while(shift > 0);

    return shift;
}

static INT32 iob_probe_cb(char *buff, void *params, INT32 size, INT32 remain_size) {
    INT32 len = *(INT32*)params;
    char  cache_hex[512] = {0}, cache_str[512] = {0}, c;
    INT32 i = 0, len_hex = 0, len_str = 0;

    printf("%p\t", buff);
    for(i = 0; i < len && i< remain_size; i++, buff++) {
       c = *buff;
       len_hex += snprintf(cache_hex+len_hex, 512-len_hex, "%2hhx ", c);
       if(c == '\0' || c == '\n' || c == '\t') {
           c = ' ';
       }
       len_str += snprintf(cache_str+len_str, 512-len_str, "%c", c);
    }
    printf("%-96s;  %s\n", cache_hex, cache_str);
    return len;
}


INT32 iob_probe(DIOB *iob) {
    INT32 const_32 = 32;
    DIOB_CB probe_cb = {iob_probe_cb, &const_32, sizeof(32)};

    _CHECK_PARAMS_RET(
        iob != (DIOB *)0,
        ERR_PARAM;
    )
    printf("\nIOB(at: %p):\n\t{flag: %x, size: %d, iov_num: %d, iov_len: %d,"
            "iov_offset: %d, iov_index: %d, base: %p}\n", iob,
            iob->flag, iob->size, iob->iov_num, iob->iov_len,
            iob->iov_offset, iob->iov_index, iob->base);

    return iob_proccess(iob, &probe_cb);
}

static INT32 iob_add_pre_sperator(char *iob_buff, void *params, INT32 size, INT32 remain_size) {
    *iob_buff = *(char*)params;
    return 1;
}

DIOB_CB iob_vertical_cb = {iob_add_pre_sperator, "|", 1};
