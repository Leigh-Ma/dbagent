#include "../utils/jober.h"
#include <sys/un.h>
#include <arpa/inet.h>
#define LOCAL_SOCK_PATH     "/tmp/ipc_man.sock"
#define SERVER_PORT   9898
int main() {
    jmsg_t *msg = NULL;
    int sock, tcp, ret, len;
    struct sockaddr_un un = {0}, addr = {0};
    struct sockaddr_in taddr= {0};
    char c = 1;
    struct link_info_msg ni;

    len = sizeof(struct link_info_msg) + sizeof(jmsg_t);
    msg = calloc(1,len);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOCAL_SOCK_PATH, sizeof(addr.sun_path) - 1);

    msg->msg_no = JMSG_INIT_REQ;
    msg->r_module = 0;
    msg->s_module = 5;
    msg->sender = 0;
    msg->receiver = 0;
    msg->msg_len = sizeof(struct link_info_msg);

    ni.module = 5;
    snprintf(ni.name, 32, "%s", "client");
    ni.job_num = 1;
    ni.jobs[0].jid = 13;
    snprintf(ni.jobs[0].name,  32, "%s", "hahaha");
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
    /*
    tcp = socket(AF_INET, SOCK_STREAM, 0);
    taddr.sin_family = AF_INET;
    taddr.sin_addr.s_addr = htonl(INADDR_ANY);
    taddr.sin_port = htons(9898);
    if(connect(tcp, (struct sockaddr *)&taddr, sizeof(taddr))) {
        printf("connect tcp error");
    }
    */

    while(1) {

        ret = send(sock, (void*)(char*)msg, len, 0);
        if(ret > 0) {
            printf("send to addr.sun_path %s %d bytes of data\n", addr.sun_path, ret);
        } else {
            printf("send to addr.sun_path %s error\n", addr.sun_path);
        }
        /*
        ret = send(tcp, (void*)(char*)msg, len, 0);
        if(ret > 0) {
            printf("send to tcp %s %d bytes of data\n", inet_ntoa(taddr.sin_addr), ret);
        } else {
            printf("send to addr.sun_path %s error\n", addr.sun_path);
        }
        */
        printf("print anything to continue...\n");
        scanf("%c", &c);
    }
    return 0;
}
