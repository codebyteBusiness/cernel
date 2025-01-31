IS_WSL := $(shell uname -a | grep -i 'Microsoft')

ifeq ($(OS),Windows_NT)
    QEMU = qemu-system-x86_64.exe
else
    QEMU = qemu-system-x86_64
endif

ifdef IS_WSL
	QEMU = qemu-system-x86_64.exe
endif

KERNEL_HDD = cernel.hdd

.PHONY: drun run compile clean all toolchain cleanToolchain

image: $(KERNEL_HDD)

all: $(KERNEL_HDD)

run:
	$(QEMU) -m 1G -debugcon stdio -drive file=$(KERNEL_HDD),format=raw -d int -no-reboot -no-shutdown

drun:
	$(QEMU) -m 1G -debugcon stdio -drive file=$(KERNEL_HDD),format=raw -s -S -d int -no-shutdown

$(KERNEL_HDD): compile
	rm -f $(KERNEL_HDD)
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(KERNEL_HDD)
	parted -s $(KERNEL_HDD) mklabel gpt
	parted -s $(KERNEL_HDD) mkpart primary 2048s 100%
	./echfs/echfs-utils -g -p0 $(KERNEL_HDD) quick-format 512
	./echfs/echfs-utils -g -p0 $(KERNEL_HDD) import kernel/cernel.elf cernel.elf
	./echfs/echfs-utils -g -p0 $(KERNEL_HDD) import limine.cfg limine.cfg
	./echfs/echfs-utils -g -p0 $(KERNEL_HDD) import limine/limine.sys limine.sys
	./limine/limine-install $(KERNEL_HDD)

compile:
	make -C kernel	

clean:
	rm -f $(KERNEL_HDD)
	make -C kernel clean

toolchain:
	make -C limine
	make -C echfs echfs-utils
	@ chmod +x toolchain/make_toolchain.sh
	@ ./toolchain/make_toolchain.sh

cleanToolchain:
	make -C limine clean
	make -C echfs clean
	rm -rf ./toolchain/cross
