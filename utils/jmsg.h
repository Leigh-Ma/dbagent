#ifndef _JMSG_H_
#define _JMSG_H_

#define         JMSG_NEW_LINK                   0x000000001
#define         JMSG_INIT_REQ                   0x000000002
#define         JMSG_INIT_ACK                   0x000000003

#define         JMSG_CLOSE_LINK                 0x000000004




struct job_info {
    int32_t jid;
    char    name[32];
};

struct link_info_msg {
    int32_t             module;
    char                name[32];
    int32_t             job_num;
    struct job_info     jobs[1];
};

#endif
