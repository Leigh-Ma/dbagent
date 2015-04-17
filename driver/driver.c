#include "driver.h"
INT32 dr_init(DB_DR *hdr, DB_CFG *cfg) {
    if(hdr == (DB_DR *)0) {
       return -1;
    }

    return hdr->opr->dr_init(hdr, cfg);
}

DB_CON *dr_new_connector(DB_DR *hdr) {
    void    *db_con = (void*)0;
    DB_CON  *hdc = (DB_CON*)0;

    if(hdr == (DB_DR *)0) {
        return (DB_CON *)0;
    }

    db_con = hdr->opr->dr_connector(hdr, hdc);
    if(db_con == (void*)0) {
        return  (DB_CON *)0;
    }

    hdc = malloc(sizeof(DB_CON));
    if(hdc == (DB_CON *)0) {
        return (DB_CON *)0;
    }

    memset(hdc, 0x00, sizeof(DB_CON));
    hdc->iob = iob_alloc(8);
    if(hdc->iob == (DIOB*)0) {
        hdr->opr->co_close(db_con);
        free(hdc);
        return (DB_CON*)0;
    }

    dr_lock(hdr);
    hdc->next = hdr->connections;
    hdr->connections = hdc;
    hdr->links += 1;
    dr_unlock(hdr);

    return hdc;
}

INT32 co_connect(DB_CON *hdc) {
    INT32   status;
    DB_CON  *n, *m;

    if(hdc == (DB_CON*)0) {
        return -1;
    }

    status = hdc->driver->opr->co_connect(hdc);
    if(0 == status) {
        dr_lock(hdc->driver);
        hdc->driver->linked += 1;
        hdr_unlock(hdc->driver);
    }

    return status;
}

INT32 co_close(DB_CON *hdc) {
    DB_DR   *hdr;
    DB_CON  *n, *m;

    if(hdc == (DB_CON*)0) {
        return -1;
    }
    hdr = hdc->driver;

    dr_lock(hdr);                      /* delete from connection list   */
    n = hdr->connections;
    hdr->connections = hdc->next;
    m = hdc;
    while(m->next) m = m->next;
    m->next = n;
    hdr->links -= 1;
    dr_unlock(hdr);

    hdc->driver->opr->co_close(hdc);
    free(hdc->iob);
    free(hdc);

    return 0;
}

INT32 dr_end(DB_DR *hdr) {
    if(hdr == (DB_DR*)0) {
        return -1;
    }
    return hdr->opr->dr_end(hdr);
}

