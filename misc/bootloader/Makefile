ARMGNU ?= arm-none-eabi

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 

all : kernel.img 

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.hex
	rm -f *.elf
	rm -f *.list
	rm -f *.img

vectors.o : vectors.s
	$(ARMGNU)-as vectors.s -o vectors.o

loader.o: loader.c
	$(ARMGNU)-gcc $(COPS) -c loader.c -o loader.o

peripherals.o: peripherals.c 
	$(ARMGNU)-gcc $(COPS) -c peripherals.c -o peripherals.o

kernel.img : link.ld vectors.o peripherals.o loader.o 
	$(ARMGNU)-ld vectors.o peripherals.o loader.o -T link.ld -o loader.elf
	$(ARMGNU)-objdump -D loader.elf > loader.list
	$(ARMGNU)-objcopy loader.elf -O binary kernel.img