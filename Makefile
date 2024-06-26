# $@ = target file
# $< = first dependency
# $^ = all dependencies

# First rule is the one executed when no parameters are fed to the Makefile
# yo jdh's makefile saved this last minute 😭
# and so did Frosnet's FrOS https://github.com/FRosner/FrOS/tree/minimal-c-kernel

# also sorry people on windows, but you need to modify this
# TODO: make this work on windows
QEMU = qemu-system-i386
GCC = i686-elf-gcc
LD = i686-elf-ld
CAT = cat
RM = rm -rf
NASM = nasm
DD = dd
MKDIR = mkdir

NASMFLAGS = -f elf
LDFLAGS = -m elf_i386 -Ttext 0x1000 --oformat binary
CCFLAGS  = -m32 -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
CCFLAGS += -Wno-pointer-arith -Wno-unused-parameter
CCFLAGS += -nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector
CCFLAGS += -fno-builtin-function -fno-builtin

QEMUFLAGS = -monitor stdio -d guest_errors -D qemu.log -no-reboot -no-shutdown

# There should only be one boot sector file. Dependencies can be called on within it.
BOOTSECT_SOURCES = boot.asm

KERNEL_SOURCES_C = $(wildcard *.c)
KERNEL_SOURCES_ASM = $(filter-out $(BOOTSECT_SOURCES), $(wildcard *.asm))
KERNEL_OBJECTS = $(patsubst %.c, build/%.o, $(KERNEL_SOURCES_C)) $(patsubst %.asm, build/%.o, $(KERNEL_SOURCES_ASM))

BOOTSECT = build/boot.bin
KERNEL = build/kernel.bin
ISO = build/out/NetOS.iso

all: clean build run

dirs:
	@$(MKDIR) -p build
	@$(MKDIR) -p build/out

build: dirs $(ISO)
	@echo "\033[32;6mBuild complete. ISO is in build/out/NetOS.iso\033[0m"

run: build
	$(QEMU) -fda $(ISO) ${QEMUFLAGS}

clean:
	$(RM) ./**/*.bin ./**/*.o ./**/*.dis ./**/*.iso ./**/*.log ./**/*.map ./*.o ./*.bin ./*.dis ./*.iso ./*.log ./*.map build

$(ISO): $(BOOTSECT) $(KERNEL)
	$(DD) if=/dev/zero of=$(ISO) bs=512 count=2880
	$(DD) if=$(BOOTSECT) of=$(ISO) conv=notrunc bs=512 seek=0 count=1
	$(DD) if=$(KERNEL) of=$(ISO) conv=notrunc bs=512 seek=1 count=2879

$(BOOTSECT): $(BOOTSECT_SOURCES)
	$(NASM) -fbin $^ -o $@

$(KERNEL): $(KERNEL_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

build/%.o: %.c
	$(GCC) -c $< -o $@ $(CCFLAGS)

build/%.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

