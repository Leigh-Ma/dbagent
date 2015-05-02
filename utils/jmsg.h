#ifndef _JMSG_H_
#define _JMSG_H_

/*management*/
#define         JMSG_NEW_THREAD                 0x00000001
#define         JMSG_NEW_LINK                   0x00000002

/*link*/
#define         JMSG_INIT_REQ                   0x00010001
#define         JMSG_INIT_ACK                   0x00010002
#define         JMSG_CLOSE_LINK                 0x00010003

#define         JMSG_TIMERS                     0xE0000000
#define         JMSG_TIMER_1                    (JMSG_TIMERS + 1)
#define         JMSG_TIMER_2                    (JMSG_TIMERS + 2)
#define         JMSG_TIMER_3                    (JMSG_TIMERS + 3)
#define         JMSG_TIMER_4                    (JMSG_TIMERS + 4)
#define         JMSG_TIMER_5                    (JMSG_TIMERS + 5)
#define         JMSG_TIMER_6                    (JMSG_TIMERS + 6)


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
