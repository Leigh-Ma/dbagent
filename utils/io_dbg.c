#include "io.h"

int main() {
    DIOB *iob, *cp;
    char *p = "abcedfghijklmnopqrstuvwxyz", c ;
    INT32  i;
    iob  = iob_alloc(1);
    for(i = 0; i < 287; i++) {
        for(c = 'a'; c <= 'd'; c++) {
            if(iob_cache(iob, &c, 1, IOBF_CACHE_BIN, &iob_vertical_cb)) {
                printf("ion_cache for %3s error\n", p);
            }
        }
    }
    //iob_probe(iob);
    cp   = iob_copy(iob);
    iob_probe(cp);
    iob = iob_release(iob);
    //iob_probe(iob);
    iob_destroy(iob);
    iob_destroy(cp);

    return 0;
}
