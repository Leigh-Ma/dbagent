#include "../pub/pub.h"
#include "jober.h"

static job_t g_main_job;

#ifdef _JOB_RECYCLE_MSG_BUFFER_
static jmsg_t *j_msg_recycle(job_t *job, int32_t len) {
    int32_t old_len = sizeof(jmsg_t);
    jmsg_t *msg = NULL;

    if(len > job->msg_recycle_len &&
            job->msg_recycle_miss > _MSG_RECYCLE_MISS_TIME_MAX_) {
        _lock_job(job);
        if(job->msg_recycle) {
            free(job->msg_recycle);
            job->msg_recycle     = NULL;
            job->msg_recycle_len = -1;
        }
        _unlock_job(job);
        return NULL;
    }

    _lock_job(job);
    if(job->msg_recycle_len >= len) {
        msg = job->msg_recycle;
        old_len += job->msg_recycle_len;
        job->msg_recycle = NULL;
        job->msg_recycle_len = -1;
        job->msg_recycle_miss--;
    } else {
        job->msg_recycle_miss++;
    }
    _unlock_job(job);
    if(msg) {
        memset(msg, 0x00, old_len);
    }
    return msg;
}
#endif

static inline void j_destroy_msg(jmsg_t *msg) {
    memset(msg, 0x00, msg->msg_len + sizeof(jmsg_t));
    free(msg);
    return;
}

static void j_msg_reclaim(job_t *job, jmsg_t *msg) {
#ifdef _JOB_RECYCLE_MSG_BUFFER_
    _lock_job(job);
    if(job->msg_recycle == NULL) {
        job->msg_recycle = msg;
        job->msg_recycle_len = msg->msg_len;
        _unlock_job(job);
    } else {
        _unlock_job(job);
#else
    {
#endif
        j_destroy_msg(msg);
    }

    return;
}

static jmsg_t *j_make_msg(job_t *sender, job_t *receiver, int32_t msgno, void *content, int32_t len) {

    jmsg_t  *msg = NULL;

#ifdef _JOB_RECYCLE_MSG_BUFFER_
    j_msg_recycle(receiver, len);
#endif

    msg = msg ? msg : calloc(1, len + sizeof(jmsg_t));
    if(msg == NULL) {
        return NULL;
    }

    msg->sender   = sender->jid;
    msg->receiver = receiver->jid;
    msg->msg_no   = msgno;
    msg->msg_len  = len;
    if(content && len > 0) {
        memcpy(msg->content, content, len);
    }

    return msg;
}


static int32_t j_push_msg(job_t *job, jmsg_t *msg) {

    _lock_job(job);
    if(job->msg_tail) {
        job->msg_tail->next = msg;
    } else {
        job->msg_queue = msg;
    }
    job->msg_tail = msg;
    job->msg_num++;
    _unlock_job(job);
    job->received_num++;
    return 0;
}

static jmsg_t *j_pop_msg(job_t *job) {
    jmsg_t *msg = NULL;

    _CHECK_PARAMS_RET(job, NULL);

    _lock_job(job);
    if(job->msg_queue) {
        msg = job->msg_queue;
        job->msg_queue = msg->next;
        if(msg == job->msg_tail) {
            job->msg_tail = NULL;
        }
        job->msg_num--;
    }
    _unlock_job(job);
    job->processed_num++;
    return msg;
}

static jmsg_t *j_peak_msg(job_t *job) {
    jmsg_t *msg = NULL;

    _CHECK_PARAMS_RET(job, NULL);

    _lock_job(job);
    msg = job->msg_queue;
    _unlock_job(job);

    return msg;
}

static int32_t j_recv_cb(job_t *me, void* params, int32_t size) {
    return read(me->read_fd, params, size);
}

static int32_t j_sendto_cb(job_t *me, void* params, int32_t size) {
    return write(me->write_fd, params, size);
}

static inline jtimer_t *j_timer_reclaim(job_t *job, int32_t timerno) {
    jtimer_t *timer = job->timers, *last;
    for(last = timer; timer != NULL; timer = timer->next) {
        if(timer->timerno == timerno) {
            if(timer == job->timers) {
                job->timers = timer->next;
            } else {
                last->next = timer->next;
            }
            break;
        }
    }
    if(timer) {
        timer->next = job->timers_recycle;
        job->timers_recycle = timer;
    }
    return timer;
}

static void job_timer_process(int fd, short which, void *arg) {
    jtimer_t *timer = (jtimer_t *)arg;
    job_t    *me = timer->job;

    if(timer->type == JOB_TIMER_LOOP) {
        timer->time.tv_sec = timer->seconds;
        evtimer_add(timer->event, &timer->time);
    } else {
        event_del(timer->event);
        j_timer_reclaim(me, timer->timerno);
    }
    me->state_machine(timer->timerno, NULL, 0, me->jid);
    me->timer_pending--;
}

static void job_event_process(int fd, short which, void *arg) {
    job_t   *me = (job_t*)arg;
    jmsg_t  *msg;
    int32_t msgno;

    if(me->before_recv && me->before_recv(me, &msgno, sizeof(msgno)) < 0) {
        return;
    }

    msg = j_pop_msg(me);
    if(msg) {
        me->state_machine(msg->msg_no, msg->content, msg->msg_len, msg->sender);
        j_msg_reclaim(me, msg);
    }

    return;
}


static void *job_thread_entry(void *arg) {
    job_t *me = (job_t*)arg;

    event_base_loop(me->base, 0);
    return NULL;
}


static int32_t job_setup(job_t* me) {
    int fd[2] = {0};

    if(me == NULL || me->state_machine == NULL || me->jid <=0) {
        return ERR_JOB_DATA;
    }
    me->base = event_init();
    if (me->base == NULL) {
        return ERR_JEV_BASE;
    }

    if(socketpair(AF_LOCAL, SOCK_STREAM, 0, fd)) {
        return ERR_JOB_SCOK;
    }
    me->read_fd = fd[0];
    me->write_fd = fd[1];
    pthread_mutex_init(&me->lock, 0);
    me->before_recv  = j_recv_cb;
    me->after_sendto = j_sendto_cb;

#ifdef _JOB_RECYCLE_MSG_BUFFER_
    me->msg_recycle_len = -1;
#endif

    event_assign(&me->notify_event, me->base, me->read_fd, EV_READ | EV_PERSIST, job_event_process, me);
    event_base_set(me->base, &me->notify_event);
    if (event_add(&me->notify_event, 0) == -1) {
        return ERR_JEV_ADD;
    }

    return 0;
}

static inline jtimer_t *j_make_timer() {
    jtimer_t *timer = malloc(sizeof(jtimer_t));

    if(timer == NULL) {
        return NULL;
    }

    memset(timer, 0x00, sizeof(jtimer_t));
    return timer;
}



static jtimer_t *j_get_timer(job_t *job) {
    jtimer_t    *timer;
    if(job->timers_recycle) {
        timer = job->timers_recycle;
        job->timers_recycle = timer->next;
        event_set(timer->event, -1, 0, job_timer_process, job);
    } else {
        timer = j_make_timer();
        if(timer) {
            timer->event = evtimer_new(job->base, job_timer_process, job);
            timer->job = job;
            job->timer_num++;
        }
    }
    return timer;
}

job_t *job_find(int32_t jid) {
    job_t *job = g_jobs;

    if(jid == 0) {
        return &g_main_job;
    }

    for( ; job != NULL ; job = job->next ) {
        if(job->jid == jid) {
            break;
        }
    }

    return job;
}

job_t *job_self() {
    job_t       *job = g_jobs;
    pthread_t   tid = pthread_self();

    for( ; job != NULL ; job = job->next ) {
        if(job->tid == tid) {
            return job;
        }
    }

    return &g_main_job;
}

int32_t job_settimer(int32_t timerno, uint32_t second, int32_t type) {
    job_t       *job = job_self();
    jtimer_t    *timer;

    if( (second <= 0 && type == JOB_TIMER_LOOP) ||
       (type != JOB_TIMER_LOOP && type == JOB_TIMER_ONCE)) {
        return ERR_JTM_PARAM;
    }

    if(second <= 0) {
        return job_asend(job->jid, timerno, NULL, 0);
    }

    timer = j_get_timer(job);             /*event is binded to event_base*/
    if(timer == NULL) {
        return ERR_JTM_CREATE;
    }

    timer->time.tv_sec  = second / 1000;
    timer->time.tv_usec = 0;
    timer->type         = type;
    timer->timerno      = timerno;
    evtimer_add(timer->event, &timer->time);
    job->timer_pending++;

    return 0;
}

int32_t job_start_work() {
    pthread_attr_t  attr;
    job_t *job = g_jobs, *last = NULL;
    int32_t i, status;

    for(i = 0, last = job; i < g_jobs_num; i++) {
        job = &g_jobs[i];
        if( 0 != (status = job_setup(job)) ) {
            exit(status);
        }
        pthread_attr_init(&attr);
        if (pthread_create(&job->tid, &attr, job_thread_entry, job)) {
            exit(ERR_JOB_CREATE);
        }
        if(last) {
            last->next = job;
        }
        last = job;
    }

    return 0;
}

static void dsipatch_timeout_cb(int fd, short which, void *arg) {
    struct event *ev = (struct event*)arg;
    struct timeval tv = {31, 0};
    static char call_time = 0;
    int32_t i = 0;

    if(call_time++ == 0) {
        for(; i < g_jobs_num; i++) {
            job_asend(g_jobs[i].jid, 99999, NULL, 0);
        }
    }
    evtimer_add(ev, &tv);
    job_probe();
    //pthread_join(g_jobs[0].tid, NULL);

    return;
}


void job_dispatch() {
    struct timeval tv = {1, 0};
    job_t  *job = job_self();

    job->base = event_base_new();
    event_assign(&job->notify_event,job->base, -1 , 0, dsipatch_timeout_cb, &job->notify_event);
    evtimer_add(&job->notify_event, &tv);
    event_base_dispatch(job->base);
}

int32_t job_asend(int32_t recv_jid, int32_t msgno, void *content, int32_t len) {
    job_t   *recv_job, *send_job;
    jmsg_t  *msg;

    recv_job = job_find(recv_jid);
    send_job = job_self();


    if(recv_job == NULL || send_job == NULL) {
        return ERR_JOB_JID;
    }

    len = len < 0 ? 0 : len;
    msg = j_make_msg(send_job, recv_job, msgno, content, len);
    if(msg == NULL) {
        return ERR_JOB_MEM;
    }

    j_push_msg(recv_job, msg);

    /* to activate a read event for the receiver thread */
    if(recv_job->after_sendto) {
        recv_job->after_sendto(recv_job, &msgno, sizeof(msgno));
    }

    return 0;
}

void job_probe() {
    job_t *job = g_jobs;

    printf("%10s\t %10s\t %10s\t %10s\t %10s\t timer_pending\n", "JOB", "request", "processed", "pending", "timers");
    for(; job != NULL ; job = job->next) {
        printf("%10s\t %10d\t %10d\t %10d\t %10d\t %10d\n", job->name, job->received_num, job->processed_num,
                job->msg_num, job->timer_num, job->timer_pending);
    }
    printf("***********************************************************************************************\n");
}


