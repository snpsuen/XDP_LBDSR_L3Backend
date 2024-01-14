## Layer 3 DSR by XDP-based Load Balancer

This is a continuation of our work on XDP-based load balancing with direct server return (DSR), [(see here)](https://github.com/snpsuen/XDP_LBDSR_Enhance). In this particular use case, the backend servers are NOT located on the same subnet as the load balancer. In other words, they are more than one subnet or hop away from where the workloads are dispatched.

To this end, the data plane is implemented on a novel architecture that involves cooperation between two XDP bpf programs attached to the load balaner and backend servers respectively. The setup allows us to focus excluively on the ingress traffic and thus make full use of the XDP hooks to fast-track workload redirection.

![XDP Load Balancer with L3 DSR](XDP_L3DSR_LoadBalancer01.png)

### Data Plane Architecture

The data plane is mainly drven by two distributed XDP bpf programs running in the kernel space of the load balancer and backend servers respectively. They work together to provide end to end clent/server communication for dispatch and delivery of workloads.
* Load balancer: xdp_lbr.epf
* Backend servers: xdp_bkd.ebf

(A) Key things done by <em> xdp_lbr.epf </em> at the load balancer upon recipt of the relevent packets
1. Select one of the backend servers
2. Rewrite the destinaton IP from the VIP to the selected server
3. Look up the next hop for the selected server in the FIB kenrel routing table
4. Forward the packets to the found next hop via XDP_TX

(B) Key things done by <em> xdp_bkd.epf </em> at the backend servers upon recipt of the relevent packets
1. Rewrite the destinaton IP from the selected server back to the VIP
2. Pass the packets to the network stack for the service endpoint to process via XDP_TX

### Limitations

Our primary concern is to wire up the XDP hooks on the data plane in such a way that any VIP service traffic will be steered from a client to a desirable endpoint and back through a DSR path. At present, we opt to leave out the control plane and use bpftool intead to load and attach the XDP bpf programs on the systems concerned. Settings about the load balancer and backend servers are also hardcoded into the programs.

In this example, the backend servers are always selected randomly to process client requests. We stop short of trying other load balancing algorithms or criteria.

Finally, only IPv4 is considered in this use case. Neverthess, we believe the architecture should apply equally well to IPv6.

### Environment Setup

The use case environment is set up as per the above diagram. Note that the load balancer is located on a different IP subnet from that of the backend server. In our POC lab, to speed up prototyping and testing, all the participating systems are deployed in the form of docker containers running on the same host.
* Load balancer lbdsr0a on the default subnet
* Curl client curlybox01 on the default subnet
* Router router0a attached to both the default and backend subnet
* Backend server backend0x on the backend subnet
* Backend Server backend0y on the backend subnet

### Implementation Walkthrough

Follow the steps below to set up the environment, build and test the XDP bpf programs. Here is a summary of IP addresses used in this example.
<table>
	<thead>
		<tr>
			<th scope="col">System/Network</th>
			<th scope="col">IP addresses</th>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>Default subnet</td>
			<td aligh="left">172.17.0.0/24</td>
		</tr>
		<tr>
			<td>Backend subnet</td>
			<td aligh="left">172.19.0.0/24</td>
		</tr>
    <tr>
			<td>lbdsr0a</td>
			<td aligh="left">172.17.0.2</td>
		</tr>
    <tr>
			<td>router0a</td>
			<td aligh="left">172.17.0.3<br>172.19.0.2</td>
		</tr>
    <tr>
			<td>curlybox01</td>
			<td aligh="left">172.17.0.4</td>
		</tr>
    <tr>
			<td>backend0x</td>
			<td aligh="left">172.19.0.3</td>
		</tr>
    <tr>
			<td>backend0y</td>
			<td aligh="left">172.19.0.4</td>
		</tr>
    <tr>
			<td>Virtual IP</td>
			<td aligh="left">192.168.25.10/32</td>
		</tr> 
	</tbody>
</table>

1. Create an additional docker subnet and run the containers for the load balancer, router, backend servers and client on a suitable Linux host.
```
docker run -d --privileged --name lbdsr0a -h lbdsr0a snpsuen/ebpf-xdp:v03
docker run -d --privileged --name router0a  -h router0a snpsuen/ebpf-xdp:v03
docker network create -d bridge backend-network 
docker network connect backend-network router0a	
docker run -d --privileged --name backend0x -h backend0x --network backend-network snpsuen/xdp-nginx:v01
docker run -d --privileged --name backend0y -h backend0y --network backend-network snpsuen/xdp-nginx:v01
docker run -d --privileged --name curlybox01 -h curlybox01 ferrgo/curlybox sleep infinity
```

2. Add routes to forward traffic between the default and backend subnet via router0a. 
```
docker exec lbdsr0a ip route add 172.19.0.0/24 via 172.17.0.3
docker exec curlybox01 ip route add 172.19.0.0/24 via 172.17.0.3
docker exec backend0x ip route add 172.17.0.0/24 via 172.19.0.2
docker exec backend0y ip route add 172.17.0.0/24 via 172.19.0.2
```

3. Configure L3 connectivity to the virtual IP
```
docker exec backend0x ip addr add 192.168.25.10/24 dev lo
docker exec backend0y ip addr add 192.168.25.10/24 dev lo
docker exec curlybox01 ip route add 192.168.25.10/32 via 172.17.0.2
docker exec lbdsr0a ip route add 192.168.25.10/32 via 172.17.0.3
```



