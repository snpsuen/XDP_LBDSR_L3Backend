#include "xdp_lbr.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <linux/string.h>

#define __force __attribute__((force))

struct {
        __uint(type, BPF_MAP_TYPE_HASH);
        __uint(map_flags, BPF_F_NO_PREALLOC);
        __type(key, struct five_tuple);
        __type(value, uint32_t);
        __uint(max_entries, 100000);
} forward_flow SEC(".maps");

static __always_inline uint8_t* uint2quad(uint32_t* numptr) {
        uint8_t* quad = (uint8_t*)((void*)numptr);
        return quad;
};

static __always_inline uint32_t quad2uint(uint8_t quad0, uint8_t quad1, uint8_t quad2, uint8_t quad3) {
        uint32_t result = quad0 * 256 * 256 * 256 + quad1 * 256 * 256 + quad2 * 256 + quad3;
        return result;
};

static __always_inline __u16 csum_fold_helper(__u64 csum) {
    int i;
#pragma unroll
    for (i = 0; i < 4; i++) {
        if (csum >> 16)
            csum = (csum & 0xffff) + (csum >> 16);
    }
    return ~csum;
};

static __always_inline __u16 iph_csum(struct iphdr *iph) {
    iph->check = 0;
    unsigned long long csum = bpf_csum_diff(0, 0, (unsigned int *)iph, sizeof(struct iphdr), 0);
    return csum_fold_helper(csum);
};

/* static __always_inline int ip_decrease_ttl(struct iphdr *iph) {
        u32 check = (__force u32)iph->check;
        check += (__force u32)bpf_htons(0x0100);
        iph->check = (__force __sum16)(check + (check >= 0xFFFF));
        return --iph->ttl;
} */

SEC("xdp")
int dispatchworkload(struct xdp_md *ctx) {
        void* data_end = (void*)(long)ctx->data_end;
        void* data = (void*)(long)ctx->data;

        struct ethhdr* eth = (struct ethhdr*)data;
        if ((void*)eth + sizeof(struct ethhdr) > data_end)
                return XDP_ABORTED;
        if (bpf_ntohs(eth->h_proto) != ETH_P_IP)
                return XDP_PASS;

        struct iphdr* iph = (struct iphdr*)((void*)eth + sizeof(struct ethhdr));
        if ((void*)iph + sizeof(struct iphdr) > data_end)
                return XDP_ABORTED;
        if (iph->protocol != IPPROTO_TCP)
                return XDP_PASS;

        struct tcphdr* tcph = (struct tcphdr*)((void*)iph + sizeof(struct iphdr));
        if ((void*)tcph + sizeof(struct tcphdr) > data_end)
                return XDP_ABORTED;

        if (bpf_ntohl(iph->daddr) == quad2uint(192, 168, 25, 10)) {
                struct five_tuple forward_key = {};
                forward_key.protocol = iph->protocol;
                forward_key.ip_source = bpf_ntohl(iph->saddr);
                forward_key.ip_destination = bpf_ntohl(iph->daddr);
                forward_key.port_source = bpf_ntohs(tcph->source);
                forward_key.port_destination = bpf_ntohs(tcph->dest);

                uint32_t backend;
                uint32_t* forward_backend = bpf_map_lookup_elem(&forward_flow, &forward_key);
                if (forward_backend == NULL) {
                        backend = BKX + (bpf_get_prandom_u32() % 2);
                        bpf_map_update_elem(&forward_flow, &forward_key, &backend, BPF_ANY);
                        bpf_printk("Added a new entry to the forward flow table for the backend ID %d", backend);
                }
                else {
                        backend = *forward_backend;
                        bpf_printk("Found the backend ID %d from an existing entry in the forward flow table ", backend);
                }

                iph->daddr = bpf_htonl(quad2uint(172, 19, 0, backend));
                uint8_t* destquad = uint2quad(&(iph->daddr));
                bpf_printk("Packet is to be dispatched to the backend IP Q1: %u", destquad[0]);
                bpf_printk("Packet is to be displatched the backend IP Q1.%u.%u.%u\n", destquad[1], destquad[2], destquad[3]);

                struct bpf_fib_lookup fib_params = {};
                fib_params.family = AF_INET;
                fib_params.tos = iph->tos;
                fib_params.l4_protocol = iph->protocol;
                fib_params.sport = 0;
                fib_params.dport = 0;
                fib_params.tot_len = bpf_ntohs(iph->tot_len);
                fib_params.ipv4_src = iph->saddr;
                fib_params.ipv4_dst = iph->daddr;
                fib_params.ifindex = ctx->ingress_ifindex;

                int rc = bpf_fib_lookup(ctx, &fib_params, sizeof(fib_params), 0);
                bpf_printk("Looked up relevant information in the FIB table with rc %d", rc);

                if (rc == BPF_FIB_LKUP_RET_SUCCESS) {
                        bpf_printk("Found fib_params.dmac[0-2] = %x:%x:%x", fib_params.dmac[0], fib_params.dmac[1], fib_params.dmac[2]);
                        bpf_printk("Found fib_params.dmac[3-5] = %x:%x:%x\n", fib_params.dmac[3], fib_params.dmac[4], fib_params.dmac[5]);
                        bpf_printk("Found fib_params.smac[0-2] = %x:%x:%x", fib_params.smac[0], fib_params.smac[1], fib_params.smac[2]);
                        bpf_printk("Found fib_params.smac[3-5] = %x:%x:%x\n", fib_params.smac[3], fib_params.smac[4], fib_params.smac[5]);

                        uint32_t nexthop = fib_params.ipv4_dst;
                        uint8_t* nhaddr = uint2quad(&nexthop);
                        bpf_printk("Found FIB nexthop IP Q1: %u", nhaddr[0]);
                        bpf_printk("Found FIB nexthop IP Q1.%u.%u.%u\n", nhaddr[1], nhaddr[2], nhaddr[3]);
                        bpf_printk("Found FIB ifindex %u\n", fib_params.ifindex);

                        /* ip_decrease_ttl(iph);
                        ip_decrease_ttl(iph); */

                        iph->check = iph_csum(iph);
                        /* iph->ttl--; */

                        memcpy(eth->h_dest, fib_params.dmac, ETH_ALEN);
                        memcpy(eth->h_source, fib_params.smac, ETH_ALEN);

                        /* bpf_printk("Calling fib_params_redirect ...");
                        return bpf_redirect(fib_params.ifindex, 0); */

                        uint8_t* saddr = uint2quad(&(iph->saddr));
                        uint8_t* daddr = uint2quad(&(iph->daddr));
                        bpf_printk("Before XDP_TX, packet is to be transported from the source IP Q1: %u ", saddr[0]);
                        bpf_printk("Before XDP_TX, packet is to be transported from the source IP Q1.%u.%u.%u\n", saddr[1], saddr[2], saddr[3]);
                        bpf_printk("To the destination IP Q1: %u ", daddr[0]);
                        bpf_printk("To the destination IP Q1.%u.%u.%u\n", daddr[1], daddr[2], daddr[3]);

                        bpf_printk("Before XDP_TX, from the source MAC %x:%x:%x:xx:xx:xx", eth->h_source[0], eth->h_source[1], eth->h_source[2]);
                        bpf_printk("Before XDP_TX, from the source MAC xx:xx:xx:%x:%x:%x\n", eth->h_source[3], eth->h_source[4], eth->h_source[5]);
                        bpf_printk("To the destination MAC %x:%x:%x:xx:xx:xx", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2]);
                        bpf_printk("To the destination MAC xx:xx:xx:%x:%x:%x\n", eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
                        bpf_printk("Returning XDP_TX...");

                        return XDP_TX;
                }
        }

        return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
