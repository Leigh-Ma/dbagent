#ifndef _JOBER_H_
#define _JOBER_H_

#include <event.h>


typedef struct job_msg_struct {
    int32_t                     msg_no;

    int32_t                     sender;
    int32_t                     s_module;
    int32_t                     receiver;
    int32_t                     r_module;

    int32_t                     msg_len;

    struct job_msg_struct       *next;

    char                        content[1];
}jmsg_t;

/*
 *  TODO better way to use event timer?
 * In order to free the @event allocate by event_new to drive timer,
 * we should keep track of the timer @event
 */
typedef struct job_timer_events {
    int32_t                     type;           /* loop timer or once */
    int32_t                     timerno;
    int32_t                     seconds;
    struct timeval              time;


    struct job_control_block    *job;
    struct event                *event;
    struct job_timer_events     *next;

}jtimer_t;



typedef void (job_state_machine)(int32_t msgno, void *content, int32_t len, int32_t sender_jid);
typedef int32_t (job_callback)(struct job_control_block*, void*, int32_t);

typedef struct job_control_block{
    char                        name[32];
    int32_t                     module;
    int32_t                     jid;            /* manually set                                 */
    job_state_machine           *state_machine;

    job_callback                *after_sendto;  /* called after something is send to this job   */
                                                /* always called right after push message       */
    job_callback                *before_recv;   /* called before receive a message for this job */
                                                /* always called right before pop message       */

    pthread_t                   tid;
    struct event_base           *base;
    struct event                notify_event;   /* internal notify                              */

    int                         read_fd;
    int                         write_fd;

    struct job_timer_events     *timers;
    struct job_timer_events     *timers_recycle;/*never free                                    */

    pthread_mutex_t             lock;
    jmsg_t                      *msg_queue;
    jmsg_t                      *msg_tail;
#ifdef _JOB_RECYCLE_MSG_BUFFER_
#define    _MSG_RECYCLE_MISS_TIME_MAX_    10    /* after @10 times miss, free the recycle       */
    jmsg_t                      *msg_recycle;   /* msg_recycle to avoid allocate, only one msg  */
    int32_t                     msg_recycle_len;/* msg_recycle carrier size                     */
    int32_t                     msg_recycle_miss;/* recycle_len too small times                 */
#endif

#ifdef     _JOB_IPC_MANAGEMENT_
    struct  ipc_node            *node;
#endif

    int32_t                     msg_num;        /* current  message count           */
    int32_t                     received_num;   /* received message count           */
    int32_t                     processed_num;  /* processed message count          */
    int32_t                     timer_num;      /* timer ever created               */
    int32_t                     timer_pending;  /* active timer count               */

    struct job_control_block    *next;
}job_t;

#define _lock_job(job)           pthread_mutex_lock(&(job)->lock)
#define _unlock_job(job)         pthread_mutex_unlock(&(job)->lock)


job_t  *job_self();
job_t  *job_find(int32_t jid);
int32_t job_start_work();
void    job_dispatch(struct event **evs, int32_t num);
void    job_probe();
#define JOB_TIMER_LOOP          1
#define JOB_TIMER_ONCE          0
int32_t job_settimer(int32_t timerno, uint32_t second, int32_t type);
int32_t job_asend(int32_t recv_jid, int32_t msgno, void *content, int32_t len);

#ifndef _JOB_IPC_MANAGEMENT_
extern job_t    g_jobs[];
extern int32_t  g_jobs_num;
#endif


#include "jmsg.h"
#endif
