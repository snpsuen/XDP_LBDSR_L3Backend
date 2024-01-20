CLANG = clang
CFLAGS = -g -O2 -Wall -Wextra
TARGET = tc_bkd
SEC = tc_egress

BPF_TARGET = ${TARGET:=.bpf}
BPF_C = ${BPF_TARGET:=.c}
BPF_OBJ = ${BPF_C:.c=.o}

all: $(BPF_OBJ)
	tc qdisc del dev eth0 clsact
  tc qdisc add dev eth0 clsact
	tc filter add dev eth0 egress bpf direct-action obj $(BPF_OBJ) sec $(SEC)
  tc filter show dev eth0

$(BPF_OBJ): $(BPF_C) vmlinux.h
	$(CLANG) $(CFLAGS) -target bpf -c $<

vmlinux.h:
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > $@

clean:
	tc qdisc del dev eth0 clsac
	rm -f $(BPF_OBJ)
	rm -f vmlinux.h

.PHONY: all clean

.DELETE_ON_ERROR:
.SECONDARY:
