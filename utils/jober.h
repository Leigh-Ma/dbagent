#ifndef _JOBER_C_
#define _JOBER_C_

#include <event.h>

typedef struct job_msg_struct{
    int32_t                     msg_no;
    int32_t                     sender;
    int32_t                     receiver;
    jmsg_t                      *next;

    int32_t                     msg_len;
    char                        content[1];
}jmsg_t;

typedef void (job_state_machine)(int32_t msgno, void *content, int32_t len, int32_t sender_jid);
typedef int32_t (job_callback)(struct job_control_block*, void*, int32);

typedef struct job_control_block{
    char                        name[32];
    int32_t                     jid;            /* manually set     */
    job_state_machine           *state_machine;

    job_callback                *send_cb;
    job_callback                *recv_cb;

    job_callback                *read_cb;
    job_callback                *write_cb;

    pthread_t                   tid;
    struct event_base           *base;
    struct event                notify_event;   /* internal notify  */

    int                         read_fd;
    int                         write_fd;

    pthread_mutex_t             lock;
    jmsg_t                      *msg_queue;
    jmsg_t                      *msg_tail;
    int32_t                     msg_num;

    struct job_control_block    *next;
}job_t;

#define _lock_job(job)           pthread_mutex_lock(job->lock)
#define _unlock_job(job)           pthread_mutex_unlock(job->lock)


extern job_t    g_jobs[];
extern int32_t  g_jobs_num;
#endif
