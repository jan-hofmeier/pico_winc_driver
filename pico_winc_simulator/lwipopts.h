#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Raw API only
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// General settings
#define NO_SYS                      1
#define LWIP_IGMP                   1
#define LWIP_ICMP                   1
#define LWIP_SNMP                   0
#define LWIP_DNS                    1
#define LWIP_HAVE_LOOPIF            1

// Memory settings
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24

// TCP settings
#define LWIP_TCP                    1
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            16

// UDP settings
#define LWIP_UDP                    1

// Enable SO_RCVTIMEO processing (used by some internal things maybe, but not sockets)
#define LWIP_SO_RCVTIMEO            0

// Fix for timeval redefinition (always good to have)
#define LWIP_TIMEVAL_PRIVATE        0

#define SYS_LIGHTWEIGHT_PROT        1

#endif /* _LWIPOPTS_H */
