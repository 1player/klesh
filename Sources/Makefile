OBJS := start.o x86.o main.o console.o cpu.o interrupt.o timer.o dma.o panic.o syscalls.o

OBJS += Misc/memory.o Misc/ll_memory.o Misc/ll_io.o Misc/string.o Misc/ll_string.o Misc/ll_64bit.o Misc/ll_bit.o

OBJS += Memory\ manager/init.o Memory\ manager/ppage.o Memory\ manager/heap.o Memory\ manager/map.o \
	Memory\ manager/dma.o

OBJS +=	Process/init.o Process/schedule.o Process/thread.o Process/elf.o Process/process.o Process/loader.o

OBJS += Modules/ata.o Modules/fdc.o Modules/cmos.o Modules/ext2.o Modules/keyboard.o

kernel.bin: $(OBJS)
	ld -n -T kernel.ld -o $@ $(OBJS)
	nm -n $@ > symbols.txt

clean:
	rm -f kernel.bin $(OBJS) symbols.txt

include Makefile.rules
