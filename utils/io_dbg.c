#include "io.h"

int main() {
    DIOB *iob, *cp;
    char p;
    INT32  i;
    iob  = iob_alloc(1);
    for(i = 0; i < 1; i++) {
        for(p = '0'; p <= 'z'; p++) {
            if(iob_cache(iob, &p, 1, IOBF_CACHE_BIN, 0)) {
                printf("ion_cache for %d %c error\n", i*('z'-'0' + 1), p);
            }
        }
    }
    iob_probe(iob);
    cp   = iob_copy(iob);
    iob_probe(cp);
    iob = iob_release(iob);
    iob_probe(iob);
    iob_destroy(iob);
    iob_destroy(cp);

    return 0;
}
