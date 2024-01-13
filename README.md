## Layer 3 DSR by XDP-based Load Balancer

This is a continuation of our work on XDP-based load balancing with direct server return (DSR), [(see here)](https://github.com/snpsuen/XDP_LBDSR_Enhance). In this particular use case, the backend servers are NOT located on the same subnet as the load balancer. In other words, they are more than one subnet or hop away from where the workloads are dispatched.

To this end, the data plane is implemented on a novel architecture that involves cooperation between two XDP bpf programs attached to the load balaner and backend servers respectively. The setup allows us to focus only on the ingress traffic and thus make full use of the XDP hooks to fast-track workload redirection.

