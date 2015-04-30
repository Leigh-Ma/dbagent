#include "../utils/jober.h"
#include <sys/un.h>
#include <arpa/inet.h>
#define LOCAL_SOCK_PATH     "/tmp/ipc_man.sock"
#define SERVER_PORT   9898
int main(int argc, char **argv) {
    jmsg_t *msg = NULL, syn = {0};
    int sock, ret, len;
    struct sockaddr_un un = {0}, addr = {0};
    char c = 1;
    struct link_info_msg ni;
    int32_t s_m, r_m;

    if(argc < 3) {
        return -1;
    }
    s_m = atoi(argv[1]);
    r_m = atoi(argv[2]);

    len = sizeof(struct link_info_msg) + sizeof(jmsg_t);
    msg = calloc(1,len);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOCAL_SOCK_PATH, sizeof(addr.sun_path) - 1);

    msg->msg_no = JMSG_INIT_REQ;
    msg->s_module = s_m;
    msg->sender = 0;
    msg->receiver = 0;
    msg->msg_len = sizeof(struct link_info_msg);

    ni.module = s_m;
    snprintf(ni.name, 32, "module-%d", s_m);
    ni.job_num = 1;
    ni.jobs[0].jid =  10 + s_m;
    snprintf(ni.jobs[0].name,  32, "%d-%d", s_m, 10 + s_m);
    memcpy(msg->content, &ni, sizeof(ni));

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    snprintf(un.sun_path, sizeof(un.sun_path), "/tmp/ipc_c%d.sock", getpid());
    unlink(un.sun_path);               /* in case it already exists */
    if (bind(sock, (struct sockaddr *)&un, sizeof(un)) < 0) {
        return 1;
    }

    ret = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    if(ret) {
        printf("connect addr.sun_path: %s to error", addr.sun_path);
        exit(-1);
    }

    ret = send(sock, (void*)(char*)msg, len, 0);
    if(ret > 0) {
        printf("send to addr.sun_path %s %d bytes of data\n", addr.sun_path, ret);
    } else {
        printf("send to addr.sun_path %s error\n", addr.sun_path);
    }

    syn.content[0] = 'a' + s_m;
    syn.msg_len = 0;
    syn.s_module = s_m;
    syn.sender   = s_m +10;
    syn.r_module = r_m;
    syn.receiver = r_m + 10;
    syn.msg_no   = 100 + s_m;
    sleep(1);
    while(1) {
        printf("Message from (%d %d) to (%d %d), content [%c]\n", syn.s_module, syn.sender, syn.r_module, syn.receiver, syn.content[0]);
        ret = send(sock, (void*)(char*)&syn, sizeof(jmsg_t), 0);

        memset(msg, 0x00, len);
        ret = recv(sock, (void*)(char*)msg, len, 0);

        printf("get message content %c, print anything to continue...\n", msg->content[0]);
        scanf("%c", &c);
    }
    return 0;
}
