#include "../pub/pub.h"
#include "jober.h"

job_t    g_jobs[10];
int32_t  g_jobs_num = sizeof(g_jobs)/sizeof(job_t);

job_t *job_find(int32_t jid) {
    job_t *job = g_jobs;

    for(; job != NULL; job = job->next) {
        if(job->jid == jid) {
            return job;
        }
    }

    return NULL;
}

job_t *job_self() {
    job_t       *job = g_jobs;
    pthread_t   tid = pthread_self();

    for(; job != NULL; job = job->next) {
        if(job->tid == tid) {
            return job;
        }
    }
    exit(-1);
}

static jmsg_t *job_make_msg(int32_t sender, int32_t receiver, int32_t msgno, void *content, int32_t len) {
    jmsg_t  *msg = calloc(1, len + sizeof(jmsg_t));

    if(msg == NULL) {
        return NULL;
    }

    msg->sender = sender;
    msg->receiver = receiver;
    msg->msg_no = msgno;
    msg->msg_len = len;
    if(content && len > 0) {
        memcpy(msg->content, content, len);
    }

    return msg;
}


static int32_t job_push_msg(job_t *job, jmsg_t *msg) {

    _lock_job(job);
    if(job->msg_tail) {
        job->msg_tail->next = msg;
    } else {
        job->msg_queue = msg;
    }
    job->msg_tail  = msg;
    job->msg_num  += 1;
    _unlock_job(job);

    return 0;
}

static jmsg_t *job_pop_msg(job_t *job) {
    jmsg_t *msg = NULL;

    _CHECK_PARAMS_RET(job, NULL);

    _lock_job(job);
    if(job->msg_queue) {
        msg = job->msg_queue;
        job->msg_queue = msg->next;
        if(msg == job->msg_tail) {
            job->msg_tail = NULL;
        }
        job->msg_num -= 1;
    }
    _unlock_job(job);

    return msg;
}

static jmsg_t *job_peak_msg(job_t *job) {
    jmsg_t *msg = NULL;

    _CHECK_PARAMS_RET(job, NULL);

    _lock_job(job);
    msg = job->msg_queue;
    _unlock_job(job);

    return msg;
}

int32_t job_asend(int32_t recv_jid, int32_t msgno, void *content, int32_t len) {
    job_t   *recv_job, *send_job;
    jmsg_t  *msg;

    recv_job = job_find(recv_jid);
    send_job = job_self();


    if(recv_job == NULL || send_job == NULL) {
        return ERR_JOB_JID;
    }

    msg = job_make_msg(send_job->jid, recv_jid, msgno, content, len);
    if(msg == NULL) {
        return ERR_JOB_MEM;
    }

    job_push_msg(recv_jid, msg);
    if(recv_job->recv_cb) {
        recv_job->recv_cb(recv_job, &msgno, sizeof(msgno));
    }

    if(send_job) {
        send_job->send_cb(send_job, &msgno, sizeof(msgno));
    }

    return 0;
}

static void job_event_process(int fd, short which, void *arg) {
    job_t   *me = (job_t*)arg;
    jmsg_t  *msg;
    int32_t msgno;

    if(0 > me->read_cb(me, &msgno, sizeof(msgno))) {
        return;
    }

    msg = job_pop_msg(me);
    me->state_machine(msg->msg_no, msg->content, msg->msg_len, msg->sender);

    return;
}

static void *job_thread_entry(void *arg) {
    job_t *me = (job_t*)arg;

    event_base_loop(me->base, 0);
    return NULL;
}
static int32_t job_setup(job_t* me) {
    int fd[2];

    me->base = event_init();
    if (me->base == NULL) {
        return ERR_JEV_BASE;
    }

    if(socketpair(AF_LOCAL, SOCK_STREAM, 0, fd)) {
        return ERR_JOB_SCOK;
    }
    me->read_fd = fd[0];
    me->write_fd = fd[1];

    event_assign(&me->notify_event, me->base, me->read_fd, EV_READ | EV_PERSIST, job_event_process, me);
    event_base_set(me->base, &me->notify_event);
    if (event_add(&me->notify_event, 0) == -1) {
        return ERR_JEV_ADD;
    }

    return 0;
}

int32_t job_start_work() {
    pthread_attr_t  attr;
    job_t *job = g_jobs;
    int32_t i, status;

    for(i = 0; i < g_jobs_num; i++, job++) {
        if( 0 == (status = job_setup(job)) ) {
            exit(status);
        }
        pthread_attr_init(&attr);
        if (pthread_create(&job->tid, &attr, job_thread_entry, job)) {
            exit(ERR_JOB_CREATE);
        }
    }

    return 0;
}



