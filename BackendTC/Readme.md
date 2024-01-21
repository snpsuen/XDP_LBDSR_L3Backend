### eBPF/TC programming in the backend

In this alternative approach to DSR from an L3-connected backend, an ebpf program runs at the Traffic Control (TC) on the backend servers. Its function is to change the source IP of the egress packets back to the virtual IP before passing them to the native network stack via TC_AT_OK to return to the client.

Just follow the same implementation procedure as the XDP-only counterpart,[(see here)](https://github.com/snpsuen/XDP_LBDSR_L3Backend), except in the backend server 
