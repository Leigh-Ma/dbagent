#include "../pub/pub.h"

#define _JOB_IPC_MANAGEMENT_
#include "../utils/jober.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/un.h>

#define IPC_NODE_UNIX       0x000000001
#define IPC_NODE_TCP        0x000000002
#define IPC_NODE_MASTER     0x000100000
#define IPC_NODE_SLAVE      0x000200000
#define IPC_NODE_WORKING    0x001000000

struct ipc_node{
    int32_t                     type;           /* IPC_UNIX | IPC_TCP               */
    int                         unix_socket;
    int                         tcp_socket;
    struct sockaddr             unix_addr;
    struct sockaddr             tcp_addr;
    pthread_mutex_t             send_lock;      /* ONLY: send to socket              */

    int32_t                     module;
    char                        name[32];

    int32_t                     *remote_jids;   /* jobs on remote peer of this node  */
    int32_t                     jid_num;        /* length of remode_jids             */
    job_t                       job;            /* local dispatching job             */
    struct event_base           *base;          /* point to job.base                 */
    struct event                timer;

    struct ipc_node             *next;
};

struct ipc_node *master = NULL;
#define SERVER_PORT   9898
#define LOCAL_SOCK_PATH     "/tmp/ipc_man.sock"

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
    job_t *job = &master->job;

    for( ; job != NULL ; job = job->next ) {
        if(job->jid == jid) {
            break;
        }
    }

    return job;
}

job_t *job_self() {
    job_t       *job = &master->job;
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
    job_t *job = &master->job;

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


static struct ipc_node *make_ipc_node(char *name) {
    struct ipc_node *node = calloc(1, sizeof(struct ipc_node));

    if(node == NULL) {
        return NULL;
    }

    if(job_setup(&node->job)) {
        free(node);
        return NULL;
    }
    pthread_mutex_init(&node->send_lock, 0);
    node->base = node->job.base;
    if(name) {
        strncpy(node->name, name, sizeof(node->name) - 1);
    }

    return node;
}

static inline  char *strify_sock_addr(struct sockaddr *addr) {
    return "";
}


static void slave_timeout_process(int fd, short which, void *arg) {

}

static void master_timeout_process(int fd, short which, void *arg) {
    struct ipc_node *node = (struct ipc_node*)arg;
    /* maintain the nodes */
    printf("Master timeout....\n");
    job_probe();
    return;
}


static struct ipc_node *node_of_model(int32_t module) {
    struct ipc_node *node = master;

    for(; node; node = node->next) {
        if(node->module == module) {
            break;
        }
    }
    return node;
}

static int32_t slave_try_read_all(int fd, void *buff, int len) {
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
                printf("Try read message (length %d) from socket(%d) error: %d\n", len, fd, err);
                return ERR_SOCK_RECV;
            }
        } else {
            read_bytes += once_read;
        }
    }
    return 0;
}

static int32_t slave_try_send_all(int fd, void *buff, int len) {
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

static int32_t slave_dispacth_socket_msg(jmsg_t *msg) {
    struct ipc_node *node = node_of_model(msg->r_module);
    int             sock;
    struct sockaddr *addr;

    if(node == NULL) {
        /* TODO error notify */
        return -1;
    }
    msg->msg_no   = ntohl(msg->msg_no);
    msg->msg_len  = ntohl(msg->msg_len);
    msg->s_module = ntohl(msg->s_module);
    msg->sender   = ntohl(msg->sender);
    msg->receiver = ntohl(msg->receiver);
    msg->r_module = ntohl(msg->r_module);

    if(node->type & IPC_NODE_UNIX) {
        sock = node->unix_socket;
        addr = &node->unix_addr;
    } else {
        sock = node->tcp_socket;
        addr = &node->unix_addr;
    }

    pthread_mutex_lock(&node->send_lock);
    slave_try_send_all(sock, msg, sizeof(jmsg_t) + msg->msg_len);       /*TODO: error notify */
    pthread_mutex_unlock(&node->send_lock);

    return 0;
}

static void slave_handle_error(int32_t error, jmsg_t *msg) {

}

static void slave_socket_process(int fd, short which, void *arg) {
    struct ipc_node *node = (struct ipc_node *)arg;
    jmsg_t          *msg, peak = {0};
    int32_t         error;
    int             len = 0;

    if(fd != node->unix_socket && fd != node->unix_socket) {
        return;
    }

    printf("Node %s receive a message\n", node->name);
    len = recv(fd, &peak, sizeof(peak), MSG_PEEK);
    if(len < sizeof(peak)) {
      printf("Node %s receive message length to small\n", node->name);
      return;
    }

    msg = j_alloc_msg(ntohl(peak.msg_len));
    if(msg == NULL) {
        return ;
    }

    error = slave_try_read_all(fd, msg, peak.msg_len + sizeof(jmsg_t));
    if(error) {
        slave_handle_error(error, msg);
        return;
    }
    slave_dispacth_socket_msg(msg);
    return;
}




static void slave_inner_event_machine(int32_t msgno, void *content, int32_t len, int32_t sender_jid);
static void *start_ipc_node(void *arg);

static int32_t start_ipc_slave(struct ipc_node *slave) {

    struct ipc_node *node;
    pthread_attr_t  attr  = {0};

    while(node->next) {
        node = node->next;
    }
    node->next      = slave;
    node->job.next  = &slave->job;
    slave->job.jid  = node->job.jid - 1;
    slave->job.state_machine = slave_inner_event_machine;

    pthread_create(&slave->job.tid, &attr, start_ipc_node, (void*)slave);
    job_asend(slave->job.jid, JMSG_INIT, NULL, 0);

    printf("slave thread created\n");

    return 0;
}


static void master_socket_process(int fd, short which, void *arg) {
    struct ipc_node *node;
    struct sockaddr addr;
    socklen_t       len;
    int             min_msgsize = sizeof(jmsg_t);
    int accept_fd = accept(fd, &addr, &len);
    printf("master accept new connection %d\n", accept_fd);

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

    /* Do not check the existence of this connection node.
     * IF this is a re-link of a node, the old node data will be useless,
     * Useless node should be remove by the management thread.
     */
    node = make_ipc_node(NULL); /*TODO, check existence */

    if(node == NULL) {
        close(accept_fd);
        printf("Make IPC node for address: %s error.\n", strify_sock_addr(&addr));
        return;
    }
    switch(addr.sa_family) {
    case AF_UNIX:
        node->type |= IPC_NODE_UNIX;
        memcpy(&node->unix_addr, &addr, sizeof(addr));
        node->unix_socket = fd;
        break;
    case AF_INET:
        node->type |= IPC_NODE_TCP;
        memcpy(&node->tcp_addr, &addr, sizeof(addr));
        node->tcp_socket = fd;
        break;
    default:
        printf("Client type not support for type.\n", addr.sa_family);
        break;
    }

    node->type |= IPC_NODE_SLAVE;

    printf("Trying to start a slave node for new connection %d...\n", accept_fd);
    start_ipc_slave(node);

    return;
}

static void *start_ipc_node(void *arg) {
    struct ipc_node     *node = arg;
    struct event        *ev;
    struct timeval      tv = {1,0};
    event_callback_fn   socket_cb = NULL, timer_cb = NULL;

    if(node->type & IPC_NODE_MASTER) {
        socket_cb = master_socket_process;
        timer_cb  = master_timeout_process;
    } else {
        socket_cb = slave_socket_process;
        timer_cb  = slave_timeout_process;
    }

    if(node->type & IPC_NODE_TCP) {
        ev = event_new(node->base, node->tcp_socket, EV_READ | EV_PERSIST, socket_cb, node);
        event_add(ev, 0);
    }
    if(node->type & IPC_NODE_UNIX) {
        ev = event_new(node->base, node->unix_socket, EV_READ | EV_PERSIST, socket_cb, node);
        event_add(ev, 0);
    }

    evtimer_assign(&node->timer, node->base, timer_cb, node);
    event_add(&node->timer, &tv);

    event_base_loop(node->base, 0);

    return NULL;
}


static void master_inner_event_machine(int32_t msgno, void *content, int32_t len, int32_t sender_jid) {

}

static int32_t start_ipc_master(struct ipc_node *master) {
    struct sockaddr_in  *addr    = NULL;
    struct sockaddr_un  *unaddr  = NULL;
    struct stat         tstat;

#define error_clean(error)      exit(error)
    if(-1 >= (master->tcp_socket  = create_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) {
        error_clean(ERR_SOCK_CREATE);
    }

    if(-1 > (master->unix_socket = create_socket(AF_UNIX, SOCK_STREAM, IPPROTO_TCP))) {
        error_clean(ERR_SOCK_CREATE);
    }

    addr = (struct sockaddr_in*)(&(master->tcp_addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr   = htonl(INADDR_ANY);
    addr->sin_port   = htons(SERVER_PORT);
    if(bind(master->tcp_socket, (struct sockaddr *)addr, sizeof(struct sockaddr))) {
        error_clean(ERR_SOCK_BIND);
    }

    unaddr = (struct sockaddr_un*)(&(master->unix_addr));
    unaddr->sun_family = AF_UNIX;
    strncpy(unaddr->sun_path, LOCAL_SOCK_PATH, sizeof(unaddr->sun_path) - 1);
    if (lstat(LOCAL_SOCK_PATH, &tstat) == 0) {
        if (S_ISSOCK(tstat.st_mode)) {
            unlink(LOCAL_SOCK_PATH);
        }
    }
    if(bind(master->unix_socket, (struct sockaddr *)unaddr, sizeof(struct sockaddr_un))) {
        error_clean(ERR_SOCK_BIND);
    }

    if(listen(master->tcp_socket, 6)) {
        error_clean(ERR_SOCK_LISTEN);
    }

    if(listen(master->unix_socket, 6)) {
        error_clean(ERR_SOCK_LISTEN);
    }

    master->type = IPC_NODE_UNIX|IPC_NODE_TCP|IPC_NODE_MASTER; /*TODO*/
    master->job.state_machine = master_inner_event_machine;
    start_ipc_node(master);

#undef error_clean

    return 0;
}


int main() {
    master = make_ipc_node("master");

    if(master == NULL) {
        printf("Make ipc master node error.\n");
        exit(-1);
    }
    start_ipc_master(master);

    return 0;
}


static void slave_inner_event_machine(int32_t msgno, void *content, int32_t len, int32_t sender_jid) {
    job_t *job = job_self();
    struct ipc_node *node = job->node;
    switch(msgno) {
    case JMSG_INIT:
        break;
    }


}

