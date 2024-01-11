#include "vmlinux.h"

#ifndef BPF_MAP_TYPE_RINGBUF
#define BPF_MAP_TYPE_RINGBUF 27
#endif

#ifndef BPF_RB_FORCE_WAKEUP
#define BPF_RB_FORCE_WAKEUP 2
#endif

#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif

#ifndef AF_INET
#define AF_INET 2
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef BPF_ANY
#define BPF_ANY 0
#endif
 
#ifndef BPF_F_NO_PREALLOC
#define BPF_F_NO_PREALLOC (1U << 0)
#endif

#define QUAD2V(a, b, c, d) (unsigned int)(a * 256 * 256 * 256 + b * 256 * 256 + c * 256 + d)
#define VAL19(x) (unsigned int)(172 * 256 * 256 * 256 + 19 * 256 * 256 + 0 * 256 + x)
#define VAL17(x) (unsigned int)(172 * 256 * 256 * 256 + 17 * 256 * 256 + 0 * 256 + x)

#define LBR 2
#define RTR 3
#define CUR 4
#define BKX 3
#define BKY 4

/*
struct xdp_md {
  __u32 data;
  __u32 data_end;
  __u32 data_meta;
  __u32 ingress_ifindex;
  __u32 rx_queue_index;
  __u32 egress_ifindex;
}
*/

struct five_tuple {
    uint8_t  protocol;
    uint32_t ip_source;
    uint32_t ip_destination;
    uint16_t port_source;
    uint16_t port_destination;
}
