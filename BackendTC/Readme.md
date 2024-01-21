### eBPF/TC programming in the backend

In this alternative approach to DSR from an L3-connected backend, an ebpf program runs at the Traffic Control (TC) on the backend servers. Its function is to change the source IP of the egress packets back to the virtual IP before passing them to the native network stack via TC_AT_OK to return to the client.

Just follow the same implementation procedure as the XDP-only counterpart,[(see here)](https://github.com/snpsuen/XDP_LBDSR_L3Backend), except for the backend server, where the TC bpf program, tc_bkd.bpf.o, is built from this folder and loaded onto the backend servers intead.

```
docker exec -it backend0x bash
git clone https://github.com/snpsuen/XDP_LBDSR_L3Backend
cd XDP*/*TC
make
```

Another point of note is, there is no need to run these commands to configure the virtual IP as an loopback aliases on the backend servers.

~~docker exec backend0x ip addr add 192.168.25.10/24 dev lo~~
~~docker exec backend0y ip addr add 192.168.25.10/24 dev lo~~


