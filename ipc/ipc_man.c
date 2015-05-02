#include "../pub/pub.h"

#define _JOB_IPC_MANAGEMENT_
#include "../utils/jober.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <event.h>
#include <stdlib.h>


#define IPC_LINK_UNIX       0x00000001
#define IPC_LINK_TCP        0x00000002
#define IPC_THREAD_MASTER   0x00010000
#define IPC_THREAD_SLAVE    0x00020000

#define THREAD_LINK_MAX     10

#define trace(stringinfo)    printf("%s: %d @%s ::"stringinfo"\n", __FILE__,__LINE__,__FUNCTION__);

enum link_state {
    ls_init,                        /* right after ipc_link is allocated            */
    ls_accepted,                    /* ipc_link has been accepted                   */
    ls_synced,                      /* ipc_link has received the remote link info   */
    ls_runing,                      /* ipc_link is distributed to a ipc_thread      */
    ls_sleeping,                    /* ipc_link has not receive jobs for a long time*/
    ls_shutdown   ,                 /* ipc_link has been shutdown by remote         */
    ls_closed,                      /* ipc_link has been closed locally             */

};
enum link_type {
    lt_unix,
    lt_tcp,
};

struct ipc_link {
    enum link_state             state;  /* link state                                   */
    enum link_type              type;   /* link type, socket family                     */
    int                         socket;
    int32_t                     module; /* remote module no */
    char                        name[32];

    event_callback_fn           ev_cb;
    struct event                event;
    union {
        struct  sockaddr_un  un;
        struct  sockaddr_in  tcp;
    }                           addr;

    job_t                       *worker;

    struct ipc_thread           *thread;

    struct ipc_link             *next;      /*use in ipc_thread, ipc_thread.links*/
    struct ipc_link             *all_link;  /*used for all link,*/

    struct job_info             *remote_jobs;
    int32_t                     remote_jobnum;
};

typedef void *(pthread_fun_t)(void *);
struct ipc_thread {
    int32_t                     state;
    int32_t                     index;
    int32_t                     type;
    pthread_t                   *tid;       /*&worker.tid*/
    pthread_mutex_t             mutex;
    pthread_cond_t              cond;
    job_t                       worker;
    struct event_base           *base;      /*worker.base*/
    struct  ipc_link            *links;
    int32_t                     nlinks;
    struct ipc_thread           *next;
};

struct ipc_stat {
    struct ipc_thread           *thread;
    int32_t                     thread_num; /*including master*/
    struct ipc_link             *all_link;
    pthread_rwlock_t            rwlock;
    struct ipc_link             *module_link[1024];

};

#define new_struct(st)     (st *)calloc(1, sizeof(st))

#define static
#define SERVER_PORT   9898
#define LOCAL_SOCK_PATH     "/tmp/ipc_man.sock"
static struct ipc_stat          ipc_stat = {0};
static struct ipc_thread        *master;


static inline void j_destroy_msg(jmsg_t *msg) {
    memset(msg, 0x00, msg->msg_len + sizeof(jmsg_t));
    free(msg);
    return;
}

static void j_msg_reclaim(job_t *job, jmsg_t *msg) {
    return j_destroy_msg(msg);
}

static inline jmsg_t *j_alloc_msg(int32_t len) {
    if(len < 0) {
        return NULL;
    }
    return calloc(1, len + sizeof(jmsg_t));
}

static jmsg_t *j_make_msg(job_t *sender, job_t *receiver, int32_t msgno, void *content, int32_t len) {
    jmsg_t  *msg = j_alloc_msg(len);
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
    me->state_machine(me, NULL, timer->timerno);
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
        me->state_machine(me, msg, msg->msg_no);    /* link_state_machine */
        j_msg_reclaim(me, msg);
    }
    return;
}


static int32_t job_setup(job_t* me) {
    int fd[2] = {0};

    if(me == NULL) {
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
    me->tid = pthread_self();

    me->before_recv  = j_recv_cb;
    me->after_sendto = j_sendto_cb;

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
    job_t *job = &ipc_stat.thread->worker;

    for( ; job != NULL ; job = job->next ) {
        if(job->jid == jid) {
            break;
        }
    }

    return job;
}

job_t *job_self() {
    job_t       *job = &ipc_stat.thread->worker;
    pthread_t   tid = pthread_self();

    for( ; job != NULL; job = job->next ) {
        if(job->tid == tid) {
            break;
        }
    }
    return job;
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

    timer->time.tv_sec  = second;
    timer->time.tv_usec = 0;
    timer->type         = type;
    timer->timerno      = timerno;
    evtimer_add(timer->event, &timer->time);
    job->timer_pending++;

    return 0;
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
    job_t *job = &ipc_stat.thread->worker;

    printf("%10s\t %10s\t %10s\t %10s\t %10s\t timer_pending\n", "JOB_ID", "request", "processed", "pending", "timers");
    for(; job != NULL ; job = job->next) {
        printf("%10d\t %10d\t %10d\t %10d\t %10d\t %10d\n", job->jid, job->received_num, job->processed_num,
                job->msg_num, job->timer_num, job->timer_pending);
    }
    printf("***********************************************************************************************\n");
}


static int create_socket(int family, int type, int protocal) {
    int sfd = socket(family, type, 0);
    struct linger ling = {0, 0};
    int flags;

    if(sfd == -1) {
        return ERR_SOCK_CREATE;
    }
    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
        fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        printf("Set socket non-block error.\n");
    }
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags))) {
        printf("Set socket reuse address error.\n");
    }
    if(setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags))) {
        printf("Set socket keep alive error.\n");
    }
    if(setsockopt(sfd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling))) {
        printf("Set socket linger error.\n");
    }
    return sfd;
}

static int32_t link_try_read_all(int fd, void *buff, int len) {
    int  read_bytes = 0, once_read, err;
    int  blocked = 0, max_block = (len/4096 + 2);
    char *p = (char*) buff;

    while(len > read_bytes) {
        once_read = recv(fd, p + read_bytes, len - read_bytes, 0);
        if(once_read == -1) {
            err = errno;
            if(err == EINTR) {
                continue;
            } else if(err == EWOULDBLOCK && max_block > blocked++) {    /* ??? */
                usleep(100);
                continue;
            } else if(err == ENOTCONN) {
                 return ERR_SOCK_CON;
            } else {
                printf("Try read message (length %d) from socket(%d) error: %s\n", len, fd, strerror(err));
                return ERR_SOCK_RECV;
            }
        } else {
            read_bytes += once_read;
        }
    }
    return 0;
}

static int32_t link_try_send_all(int fd, void *buff, int len) {
    int  wrote_bytes = 0, once_wrote, err;
    int  blocked = 0, max_block = (len/4096 + 2);
    char *p = (char*) buff;

    while(len > wrote_bytes) {
        once_wrote = send(fd, p + wrote_bytes, len - wrote_bytes, 0);
        if(once_wrote == -1) {
            err = errno;
            if(err == EINTR) {
                continue;
            } else if(err == EWOULDBLOCK && max_block > blocked++) {    /* ??? */
                usleep(10);
                continue;
            } else if(err == ENOTCONN) {
                 return ERR_SOCK_CON;
            } else {
                printf("Try read message (length %d) from socket(%d) error: %d\n", len, fd, err);
                return ERR_SOCK_RECV;
            }
        } else {
            wrote_bytes += once_wrote;
        }
    }

    return 0;
}



static void link_worker_cb(int fd, short which, void *arg);
/*===================================================================================*/
static inline void STAT_W_LOCK() {
    pthread_rwlock_trywrlock(&ipc_stat.rwlock);
}
static inline void STAT_W_UNLOCK() {
    pthread_rwlock_unlock(&ipc_stat.rwlock);
}

static inline void STAT_R_LOCK() {
    pthread_rwlock_tryrdlock(&ipc_stat.rwlock);
}
static inline void STAT_R_UNLOCK() {
    pthread_rwlock_unlock(&ipc_stat.rwlock);
}

static inline void THREAD_LOCK(struct ipc_thread *thread) {
    pthread_mutex_lock(&thread->mutex);
}
static inline void THREAD_UNLOCK(struct ipc_thread *thread) {
    pthread_mutex_unlock(&thread->mutex);
}

static inline void THREAD_WAIT(struct ipc_thread *thread) {
    pthread_cond_wait(&thread->cond, &thread->mutex);
}
static inline void THREAD_SIGNAL(struct ipc_thread *thread) {
    pthread_cond_signal(&thread->cond);
}


/*call by master thread*/

static inline void thread_link_register(struct ipc_thread *thread, struct ipc_link *link) {
    link->thread = thread;
    link->worker = &thread->worker;

    THREAD_LOCK(thread);
    link->next   = thread->links;
    thread->links= link;
    thread->nlinks++;
    THREAD_UNLOCK(thread);

    STAT_W_LOCK();
    link->all_link = ipc_stat.all_link;
    ipc_stat.all_link = link;
    STAT_W_UNLOCK();
}

static inline void thread_link_unregister(struct ipc_thread *thread, struct ipc_link *link) {
    struct ipc_link *m;

    printf("Thread(index:%d, links:%d) link(socket %d, type %d, status %d): Unregister link.\n",
                     thread->index, thread->nlinks, link->socket, link->type, link->state);

    THREAD_LOCK(link->thread);
    m = link->thread->links;
    if(m != link) {
        for(; m && m->next != link; m = m->next) {
        }
        m->next = link->next;
    } else {
        link->thread->links =  link->next;
    }
    link->thread->nlinks--;
    THREAD_UNLOCK(link->thread);

    STAT_W_LOCK();
    if(link == ipc_stat.module_link[link->module]) {
        ipc_stat.module_link[link->module] = NULL;
    }

    m = ipc_stat.all_link;
    if(m && m != link) {
       for(; m && m->all_link != link; m = m->all_link) {
       }
       m->all_link = link->all_link;
    } else {
       ipc_stat.all_link =  link->next;
    }
    STAT_W_UNLOCK();
}

static struct ipc_link *thread_make_link(struct ipc_thread *thread, enum link_type type) {
    struct ipc_link *link = new_struct(struct ipc_link);

    if(link == NULL) {
        return NULL;
    }

    link->state  = ls_init;
    link->type   = type;
    thread_link_register(thread, link);

    return link;
}

static struct ipc_thread *make_ipc_thread() {
    struct ipc_thread *thread = new_struct(struct ipc_thread);
    int32_t err;

    if(thread == NULL) {
        return NULL;
    }
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    err = job_setup(&thread->worker);
    if(err) {
        return NULL;
    }
    thread->base    = thread->worker.base;
    thread->tid     = &thread->worker.tid;

    STAT_W_LOCK();
    thread->index   = ipc_stat.thread_num++;
    if(ipc_stat.thread) {
        thread->worker.next = &ipc_stat.thread->worker;
    }
    thread->next    = ipc_stat.thread;
    ipc_stat.thread = thread;
    STAT_W_UNLOCK();

    thread->worker.jid = thread->index;
    thread->worker.thread = thread;

    return thread;
}

static int32_t thread_start_monitor(struct ipc_thread *thread) {
    event_base_loop(thread->base, 0);
    return 0;
}

static int32_t thread_monitor_link(struct ipc_thread *thread, struct ipc_link *link, event_callback_fn cb) {
    printf("Thread(index:%d, links:%d) link(socket %d, type %d, status %d): start to monitor\n",
                  thread->index, thread->nlinks, link->socket, link->type, link->state);
    link->ev_cb  = cb;
    if(event_assign(&link->event, thread->base, link->socket, EV_READ | EV_PERSIST, link->ev_cb, link)) {
        trace("event assign link->event to thread base error");
    }
    if(event_add(&link->event, 0)) {
        trace("event add link->event error");
    }

    return 0;
}


static struct ipc_link *link_of_thread_msg(struct ipc_thread *thread, jmsg_t *msg) {
    struct ipc_link *link = NULL, *m;

    if(msg->msg_no == JMSG_NEW_LINK || msg->msg_no == JMSG_CLOSE_LINK) {
        if(msg->msg_len != sizeof(link)) {
           return NULL;
        } else {
            link = *(struct ipc_link **)msg->content;
        }
    }

    for(m = thread->links; m != NULL; m = m->next) {
       if((link == NULL && m->socket == msg->link_fd) || link == m) {
           break;
        }
    }

    return m;
}

static int32_t link_proc_new_link(struct ipc_link *link, jmsg_t *msg) {
    return thread_monitor_link(link->thread, link, link_worker_cb);
}

static int32_t link_proc_close_link(struct ipc_link *link) {

    close(link->socket);    /* this will cause an event, ??? actively close will not cause event? */

    if(link->remote_jobs) {
        free(link->remote_jobs);
        link->remote_jobs = NULL;
    }
    event_del(&link->event);

    thread_link_unregister(link->thread, link);
    free(link);

    return 0;
}

static int32_t link_proc_init_req(struct ipc_link *link, jmsg_t *msg) {
    struct  link_info_msg *li = (struct link_info_msg *)msg->content;
    struct  job_info      *ji;
    struct ipc_link       *old_link = NULL;
    int32_t i = 0;

    STAT_R_LOCK();
    if((old_link = ipc_stat.module_link[li->module]) != NULL) {
       /*TODO, should close the old link, or warning for module conflicts */
       printf("\tWarning for module conflicts, notify the old link(socket %d) to close\n", old_link->socket);
       job_asend(old_link->worker->jid, JMSG_CLOSE_LINK, &old_link, sizeof(old_link));
    }
    STAT_R_UNLOCK();

    link->remote_jobnum = li->job_num;
    link->module        = li->module;
    snprintf(link->name, sizeof(link->name), "%s", li->name);

    if(link->remote_jobnum) {
       link->remote_jobs   = calloc(li->job_num, sizeof(struct job_info));
       if(link->remote_jobs != NULL) {
           memcpy(link->remote_jobs, li, link->remote_jobnum * sizeof(struct job_info));
       } else {
           printf("Allocate memory error for remote jobs information.\n");
           link->remote_jobs = 0;
           return -1;
       }
    }
    STAT_W_LOCK();
    ipc_stat.module_link[li->module] = link;
    STAT_W_UNLOCK();

    printf("Thread(index:%d, links:%d) link(socket %d, type %d, status %d): module(%d, %s) has the following jobs: \n",
                  link->thread->index, link->thread->nlinks, link->socket, link->type, link->state, link->module, link->name);
    for(ji = li->jobs; i < link->remote_jobnum; i++, ji++) {
        printf("\t{jid: %d, name: %s}\n", ji->jid, ji->name);
    }

    return 0;
}

static struct ipc_link *link_of_link_msg(struct ipc_link *sender, jmsg_t *msg) {
    struct ipc_link *receiver = NULL;

    printf("Message from (%d %d) to (%d %d), content [%c]\n", msg->s_module, msg->sender, msg->r_module, msg->receiver, msg->content[0]);
    /* send to myself, management message */
    if(msg->r_module == sender->module || msg->r_module == 0) {
        return sender;
    }

    STAT_R_LOCK();
    receiver = ipc_stat.module_link[msg->r_module];
    STAT_R_UNLOCK();

    return receiver;
}

static int32_t link_proc_forward_msg(struct ipc_link *sender, jmsg_t *msg) {
    struct ipc_link *me = link_of_link_msg(sender, msg);

    if(me&&me->socket) {
        printf("Forward message [%c] to socket %d\n", msg->content[0], me->socket);
        link_try_send_all(me->socket, msg, msg->msg_len + sizeof(jmsg_t));
    }

    return 0;
}

static void link_state_machine(job_t *job, jmsg_t *msg, int32_t msgno) {
    struct ipc_link *link = link_of_thread_msg(job->thread, msg);

#define _check_link_state(link, stat)                                                     \
    if((link)->state != (stat)) {                                                         \
        printf("\tMessage %d inappropriate for link state %d\n", msgno, stat);            \
        return;                                                                           \
    }

#define _set_link_state(link, stat)   (link)->state = stat
    if(link == NULL) {
        return;
    }
    /*
     printf("Thread(index:%d, links:%d) link(socket %d, type %d, status %d): receive message %d\n",
               job->thread->index, job->thread->nlinks, link->socket, link->type, link->state, msgno);
     */
    switch(msgno) {
    case JMSG_NEW_THREAD:
        printf("JMSG_NEW_THREAD\n");
        job_settimer(JMSG_TIMER_1, 5, JOB_TIMER_LOOP);
        break;
    case JMSG_TIMER_1:
        printf("timer %x timeout\n", JMSG_TIMER_1);
        break;
    case JMSG_NEW_LINK:
        _check_link_state(link, ls_init);
        if(0 == link_proc_new_link(link, msg) ) {
            _set_link_state(link, ls_accepted);
        }
        break;
    case JMSG_INIT_REQ:
        _check_link_state(link, ls_accepted);
        if(0 == link_proc_init_req(link, msg) ) {
            _set_link_state(link, ls_synced);
        }
        break;
    case JMSG_CLOSE_LINK:
        link_proc_close_link(link);
        break;
    default:
        link_proc_forward_msg(link, msg);
        break;
    }

    return;
}

static void *ipc_thread_entry(void *arg) {
    struct ipc_thread **thread = (struct ipc_thread **)arg;

    *thread = make_ipc_thread();
    if(*thread == NULL) {
        THREAD_SIGNAL(master);
        return NULL;
    }
    (*thread)->worker.state_machine = link_state_machine;

    /* can not be put after @thread_start_monitor, it will block */
    THREAD_SIGNAL(master);

    thread_start_monitor(*thread);

    return NULL;
}


static struct ipc_thread *start_new_ipc_thread(struct ipc_thread **thread) {
    pthread_t tid;

    THREAD_LOCK(master);
    if(pthread_create(&tid, NULL, ipc_thread_entry, (void*)thread)) {
        THREAD_UNLOCK(master);
        printf("pthread_create create thread error: entry\n");
        return NULL;
    }
    /* *thread value is need to be return, so wait for the new thread to set it's value */
    THREAD_WAIT(master);
    THREAD_UNLOCK(master);

    return *thread;
}


static struct ipc_thread *thread_for_new_link(int sock, enum link_type type) {
    struct ipc_thread *thread = NULL;
    struct ipc_link   *link;

    for(link = ipc_stat.all_link; link != NULL; link = link->all_link) {
        if(link->socket == sock) {
            printf("Link exist in all link list for socket %d.\n", sock);
            return thread;
        }
    }


    /*the last allocated thread is always at the head, may be master*/
    STAT_R_LOCK();
    thread = ipc_stat.thread;
    STAT_R_UNLOCK();

    if(thread == NULL || thread->index == 0 || thread->nlinks >= THREAD_LINK_MAX) {
        thread = NULL;
        start_new_ipc_thread(&thread);
        /* FIXME: wait for the thread to call event_base_loop; then send a init message to the thread；
         * the thread will block at event_base_loop, so the init message can not be send by
         * the thread its self, this will be done by a timer
         */

    }

    return thread;
}



static int32_t link_dispacth_socket_msg(struct ipc_link *me, jmsg_t *msg) {
    struct ipc_link *aim = link_of_link_msg(me, msg);

    if(aim == NULL) {
        printf("Can not find aim link of message, aim {module: %d, job_id: %d}\n", msg->r_module, msg->receiver);
        return 0;
    }

    j_push_msg(aim->worker, msg);
    /* to activate a read event for the receiver thread */
    if(aim->worker->after_sendto) {
        aim->worker->after_sendto(aim->worker, &msg->msg_no, sizeof(msg->msg_no));
    }

    return 0;
}

static int32_t link_handle_error(struct ipc_link *me, jmsg_t *msg, int32_t error) {
    return 0;
}

static void link_worker_cb(int fd, short which, void *arg) {
    struct ipc_link    *me = ( struct ipc_link *)arg;
    jmsg_t             peak, *msg;
    int                len, error;

    assert(me->socket == fd);

    len = recv(fd, &peak, sizeof(peak), MSG_PEEK);
    if(len < sizeof(peak)) {
        if(len == 0) {
            /* len == 0 when remote end close or local host close */
            len = recv(fd, &peak, sizeof(peak), 0);
            job_asend(me->worker->jid, JMSG_CLOSE_LINK, &me, sizeof(me));
            return;
        } else {
            printf("Link for module %d receive message length to small\n", me->module);
            return;
        }
    }

    msg = j_alloc_msg(peak.msg_len);
    if(msg == NULL) {
        return ;
    }

    error = link_try_read_all(fd, msg, peak.msg_len + sizeof(jmsg_t));
    if(error) {
        link_handle_error(me, msg, error);
        return;
    }

    msg->link_fd = fd; /* who received this */
    link_dispacth_socket_msg(me, msg);
    return;

}

static void link_accept_cb(int fd, short which, void *arg) {
    struct ipc_link    *me = ( struct ipc_link *)arg, *link;
    struct sockaddr_un  addr;        /*with max size*/
    struct ipc_thread   *thread;
    socklen_t           len = sizeof(struct sockaddr_un);
    int                 min_msgsize = sizeof(jmsg_t);
    int                 accept_fd = accept(fd, (struct sockaddr *)&addr, &len);

    printf("Thread(index:%d, links:%d) link(socket %d, type %d, status %d): trying to accept new link for socket %d\n",
                  me->thread->index, me->thread->nlinks, me->socket, me->type, me->state,accept_fd);

    if(accept_fd == -1) {
        return;
    }

    if (fcntl(accept_fd, F_SETFL, fcntl(accept_fd, F_GETFL) | O_NONBLOCK) < 0) {
        close(accept_fd);
        printf("set new connection non-block error\n");
        return;
    }

    if(setsockopt(accept_fd, SOL_SOCKET, SO_RCVLOWAT, (void *)&min_msgsize, sizeof(int))) {
         printf("Set socket reuse address error.\n");
    }

    /* find or create a thread to manage the new link */

    thread = thread_for_new_link(accept_fd, me->type);
    if(thread == NULL) {
        printf("find or create thread to work for new connection %d error\n", accept_fd);
        close(accept_fd);
        return;
    }

    link   = thread_make_link(thread, me->type);
    if(link == NULL) {
        printf("find or create new link for new connection %d error\n", accept_fd);
        close(accept_fd);
        return;
    }

    link->socket = accept_fd;

    memcpy(&link->addr, &addr, sizeof(addr));
    job_asend(thread->worker.jid, JMSG_NEW_LINK, &link , sizeof(link));

    /* FIXME: accepted address for unix socket has error in sun_family  */

    /* can not add new event to a event_base in other thread directly   */
    /* thread_monitor_link(thread, link, link_worker_cb);               */

    return;
}

static int32_t  start_ipc_master(struct ipc_thread *thread) {
    struct ipc_link *unix = NULL, *tcp = NULL;
    struct stat tstat;
#define error_clean(is_error, error)  if(is_error) {printf("%d: error occur in start_ipc_master, exit(%s)\n", __LINE__, strerror(errno));exit(error); }

    unix = thread_make_link(thread, lt_unix);
    error_clean(unix == NULL, -1);

    tcp = thread_make_link(thread, lt_tcp);
    error_clean(tcp == NULL, -1);

    unix->socket = create_socket(AF_UNIX, SOCK_STREAM, 0);
    error_clean(unix->socket < 0, -1);
    unix->addr.un.sun_family = AF_UNIX;
    snprintf(unix->addr.un.sun_path, sizeof(unix->addr.un.sun_path), "%s", LOCAL_SOCK_PATH);

    if (lstat(LOCAL_SOCK_PATH, &tstat) == 0) {
        if (S_ISSOCK(tstat.st_mode)) {
            unlink(LOCAL_SOCK_PATH);
        }
    }
    error_clean(0 != bind(unix->socket, (struct sockaddr *)&unix->addr.un, sizeof(unix->addr.un)), -1);
    error_clean(0 != listen(unix->socket, 10), -1);

    tcp->socket = create_socket(AF_INET, SOCK_STREAM, 0);
    error_clean(tcp->socket < 0, -1);
    tcp->addr.tcp.sin_family = AF_INET;
    tcp->addr.tcp.sin_port   = htons(SERVER_PORT);
    tcp->addr.tcp.sin_addr.s_addr = htonl(INADDR_ANY);

    error_clean(0 != bind(tcp->socket, (struct sockaddr *)&tcp->addr.tcp, sizeof(tcp->addr.tcp)), -1);
    error_clean(0 != listen(tcp->socket, 10), -1);

    thread_monitor_link(thread, unix, link_accept_cb);
    thread_monitor_link(thread, tcp,  link_accept_cb);
    thread->worker.state_machine = link_state_machine;  /*FIXME*/
    thread_start_monitor(thread);
#undef error_clean
    return 0;
}

int main() {
    pthread_rwlock_init(&ipc_stat.rwlock, NULL);
    master = make_ipc_thread();
    if(master == NULL) {
        printf("make master thread error\n");
        exit(-1);
    }
    start_ipc_master(master);
    return 0;
}




