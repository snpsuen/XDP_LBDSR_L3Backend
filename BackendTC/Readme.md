### eBPF/TC programming in the backend

Central to this alternative approach to DSR from an L3-connected backend is an ebpf program running at the Traffic Control (TC) on the backend servers. Its function is to change the source IP of the egress packets back to the virtual IP before passing them to the native network stack via TC_AT_OK to return to the client.

Just follow the same implementation procedure as the XDP-only counterpart,[(see here)](https://github.com/snpsuen/XDP_LBDSR_L3Backend), except for the backend servers, where the TC bpf program, tc_bkd.bpf.o, is built from this folder and loaded onto the backend servers intead.

```
docker exec -it backend0x bash
git clone https://github.com/snpsuen/XDP_LBDSR_L3Backend
cd XDP*/*TC
make
```

Meanwhile, there is no need to run these commands to configure the virtual IP as an loopback alias on the backend servers.

~~docker exec backend0x ip addr add 192.168.25.10/24 dev lo~~

~~docker exec backend0y ip addr add 192.168.25.10/24 dev lo~~

Another thing is, depending on your setup environment, you probably need to ping from the load balancer container to a backend server first in order to populate the FIB kernel table appropriately. Otherwise the XDP bpf program running on the load balancer may complain there is no neighbour to forward the backend traffic to.

```
keyuser@ubunclone:~$ docker exec -it lbdsr0a bash

root@lbdsr0a:/ebpf/xdp# ping 172.19.0.3
PING 172.19.0.3 (172.19.0.3) 56(84) bytes of data.
64 bytes from 172.19.0.3: icmp_seq=1 ttl=63 time=0.214 ms
64 bytes from 172.19.0.3: icmp_seq=2 ttl=63 time=0.223 ms
64 bytes from 172.19.0.3: icmp_seq=3 ttl=63 time=0.122 ms
64 bytes from 172.19.0.3: icmp_seq=4 ttl=63 time=0.088 ms
^C
--- 172.19.0.3 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3054ms
```

After that, you are ready to test it out by issuing an HTTP request for the virtual IP in a loop from the curl client.

```
keyuser@ubunclone:~$ docker exec -it curlybox01 sh

/ #
/ # while true
> do
> curl -s http://192.168.25.10
> sleep 3
> done
Server address: 172.19.0.3:80
Server name: backend0x
Date: 22/Jan/2024:00:08:00 +0000
URI: /
Request ID: 28387552b65400aca1690d82dd40b85b
Server address: 172.19.0.4:80
Server name: backend0y
Date: 22/Jan/2024:00:08:03 +0000
URI: /
Request ID: bd67d79cb951e091751cf24ed90ecfb2
Server address: 172.19.0.3:80
Server name: backend0x
Date: 22/Jan/2024:00:08:06 +0000
URI: /
Request ID: 0cc92c15a3514029df706fecdcb30d4e
Server address: 172.19.0.3:80
Server name: backend0x
Date: 22/Jan/2024:00:08:09 +0000
URI: /
Request ID: 3b395279688c6150b22efefb0ec9fc29
Server address: 172.19.0.3:80
Server name: backend0x
Date: 22/Jan/2024:00:08:12 +0000
URI: /
Request ID: 3075ef7e2c3237b592781920b199f233
Server address: 172.19.0.4:80
Server name: backend0y
Date: 22/Jan/2024:00:08:15 +0000
URI: /
Request ID: 28176c62920c65b42c3732b27a1e9764
Server address: 172.19.0.3:80
Server name: backend0x
Date: 22/Jan/2024:00:08:18 +0000
URI: /
Request ID: 72ccec6bf479f933ea1d55eb80810afe
Server address: 172.19.0.4:80
Server name: backend0y
Date: 22/Jan/2024:00:08:21 +0000
URI: /
Request ID: 28cc1893ab68b1fae7114ba82144d2f4
Server address: 172.19.0.4:80
Server name: backend0y
Date: 22/Jan/2024:00:08:24 +0000
```
