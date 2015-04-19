#ifndef _DRIVER_MACRO_H_
#define _DRIVER_MACRO_H_

#define reset_db_response(hdc, resp, iob)     \
    if(resp) {                                \
        (resp)->row_num = -1;                 \
        iob_release((resp)->iob);             \
        iob = (resp)->iob;                    \
    }                                         \
    if(iob == (DIOB*)0) {                     \
        iob = hdc->iob;                       \
    }

#define set_db_response(resp, iob, row_num)   \
     if(resp) {                               \
        if((resp)->iob == (DIOB*)0) {         \
            (resp)->iob = iob_copy(iob);      \
        }                                     \
        (resp)->row_num = row_num;            \
    }

#endif
