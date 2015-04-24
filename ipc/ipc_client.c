#include "../utils/jober.h"
#include <sys/un.h>
#define LOCAL_SOCK_PATH     "/tmp/ipc_man.sock"
#define SERVER_PORT   9898
int main() {
    jmsg_t msg = {0};
    int sock, ret;
    struct sockaddr_un addr = {0};
    char c = 1;

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOCAL_SOCK_PATH, sizeof(addr.sun_path) - 1);

    msg.msg_no = 1234;
    msg.r_module = 1;
    msg.s_module = 2;
    msg.sender = 9;
    msg.receiver = 10;
    msg.msg_len = 0;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    ret = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    if(ret) {
        printf("connect addr.sun_path: %s to error", addr.sun_path);
        exit(-1);
    }
    while(c++ % 10) {

        ret = send(sock, &msg, sizeof(msg), 0);
        if(ret > 0) {
            printf("send to addr.sun_path %s %d bytes of data\n", addr.sun_path, ret);
        } else {
            printf("send to addr.sun_path %s error", addr.sun_path);
        }
        sleep(1);
    }
    return 0;
}
