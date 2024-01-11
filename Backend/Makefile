CLANG = clang
CFLAGS = -g -O2 -Wall -Wextra
TARGET = xdp_bkd
SEC = xdp

BPF_TARGET = ${TARGET:=.bpf}
BPF_C = ${BPF_TARGET:=.c}
BPF_OBJ = ${BPF_C:.c=.o}

all: $(BPF_OBJ)
	bpftool net detach xdpgeneric dev eth0
	rm -f /sys/fs/bpf/$(SEC)
	bpftool prog load $(BPF_OBJ) /sys/fs/bpf/$(SEC)
	bpftool net attach xdpgeneric pinned /sys/fs/bpf/$(SEC) dev eth0 

$(BPF_OBJ): $(BPF_C) vmlinux.h
	$(CLANG) $(CFLAGS) -target bpf -c $<

vmlinux.h:
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > $@

clean:
	bpftool net detach xdpgeneric dev eth0
	rm -f /sys/fs/bpf/$(SEC)
	rm -f $(BPF_OBJ)
	rm -f vmlinux.h

.PHONY: all clean

.DELETE_ON_ERROR:
.SECONDARY:
