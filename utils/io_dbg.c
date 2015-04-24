#include "../pub/pub.h"
#include "jober.h"

void job_hello(int32_t msgno, void *content, int32_t len, int32_t sender_jid);
job_t g_jobs[] = {
        {"job_hello", 1, 1, job_hello },
        {"job_hello", 1, 2, job_hello },
        {"job_hello", 1, 3, job_hello },
        {"job_hello", 1, 4, job_hello },
        {"job_hello", 1, 5, job_hello },
        {"job_hello", 1, 6, job_hello },
        {"job_hello", 1, 7, job_hello },
        {"job_hello", 1, 8, job_hello },

};

int32_t  g_jobs_num = sizeof(g_jobs) / sizeof(job_t);

void job_hello(int32_t msgno, void *content, int32_t len, int32_t sender_jid) {
    int32_t   xxx = 1;
    usleep(5000);
    job_asend(g_jobs_num - job_self()->jid + 1, 2222, &xxx, sizeof(xxx));
}


int main() {
   /* DIOB *iob, *cp;
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
    iob_destroy(cp);*/

    job_start_work();
    job_dispatch(NULL, 0);
    return 0;
}



