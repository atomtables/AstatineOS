#!/usr/bin/env bash
# got GPT to convert the cmakelists.txt to this because cmake sucks for custom comp
set -e

if [ -e ".build.log" ]; then
  source .build.log
else
  echo "Running a clean compilation."
fi
# ───────────────────────────────────────────────
# CONFIGURATION
# ───────────────────────────────────────────────

    CROSS_GCC=i686-elf-gcc
    LD=i686-elf-ld
    NASM=nasm
    DD=dd
    MV=mv
    MKDIR=mkdir
    QEMU=qemu-system-i386-unsigned
    MKABP=mkabp
    XXD=xxd

    # TODO: MUST BE CHANGED FOR LOCAL COMPILATION
    CROSS_GCC_INCLUDE=/Users/3024492/Applications/homebrew/Cellar/i686-elf-gcc/15.2.0/lib/gcc/i686-elf/15.2.0/include
    CROSS_GCC_INCLUDE_FIXED=/Users/3024492/Applications/homebrew/Cellar/i686-elf-gcc/15.2.0/lib/gcc/i686-elf/15.2.0/include-fixed

# ───────────────────────────────────────────────
# DIRECTORY STRUCTURE
# ───────────────────────────────────────────────

    SOURCE_DIR="src"
    BUILD_DIR="objects"
    BOOT_DIR="${SOURCE_DIR}/boot"
    BOOTLOADER_DIR="${SOURCE_DIR}/stage2"
    KERNEL_DIR="${SOURCE_DIR}/kernel"
    LIBRARY_DIR="${SOURCE_DIR}/libraries"
    PRODUCT_DIR="product"
    BOOTLOADER_COMP_DIR="${BUILD_DIR}/ABP"

    mkdir -p "$BUILD_DIR" "$BOOTLOADER_COMP_DIR" "$PRODUCT_DIR"

# ───────────────────────────────────────────────
# FLAGS
# ───────────────────────────────────────────────

    NASMFLAGS="-felf"
    LDFLAGS="-m elf_i386 -Ttext 0x10000 --oformat binary"
    CCFLAGS_KERNEL="-m32 -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
    -Wno-pointer-arith -Wno-unused-parameter \
    -nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector \
    -fno-builtin-function -fno-builtin
    -I${KERNEL_DIR} -I${CROSS_GCC_INCLUDE} -I${CROSS_GCC_INCLUDE_FIXED}"

    QEMUFLAGS="-monitor stdio -d int,in_asm -D qemu.log -audiodev coreaudio,id=audio0 -machine pcspk-audiodev=audio0 -debugcon file:qemu.log -no-reboot -no-shutdown"

# ───────────────────────────────────────────────
# PATHS
# ───────────────────────────────────────────────

    BOOTSECT_SRC="${BOOT_DIR}/boot.asm"
    BOOTSECT_BIN="${BUILD_DIR}/boot.bin"

    BOOTLOADER_SRC="${BOOTLOADER_DIR}/boot.aex.asm"
    BOOTLOADER_C_SRC="${BOOTLOADER_DIR}/otherfiles/fat32.c ${BOOTLOADER_DIR}/otherfiles/main.c"
    BOOTLOADER_ASM_SRC="${BOOTLOADER_DIR}/otherfiles/ata_lba_read_c.asm ${BOOTLOADER_DIR}/otherfiles/entry.asm"
    BOOTLOADER_BIN="${BUILD_DIR}/abp.bin"
    BOOTLOADER_MAIN="${BOOTLOADER_COMP_DIR}/BOOT.AEX"
    BOOTLOADER_CASM="${BOOTLOADER_COMP_DIR}/LOADER.AEX"

    KERNEL_IMG="${BUILD_DIR}/kernel.img"
    ISO_IMG="${BUILD_DIR}/NetworkOS.iso"
    FINAL_ISO="${PRODUCT_DIR}/NetworkOS.iso"
# ───────────────────────────────────────────────
# CHANGE SWITCHES
# ───────────────────────────────────────────────
KERNEL_DIR_SRC_CHANGED=false
BOOTSECTOR_SRC_CHANGED=false
BOOTLOADER_SRC_CHANGED=false
# ───────────────────────────────────────────────
# COMPILE KERNEL (C + ASM)
# ───────────────────────────────────────────────

echo "[*] Compiling kernel sources..."
find "${KERNEL_DIR}" -type f -name "*.c" | while read -r SRC; do
    val1="${SRC//\//_}"
    val="${val1//./_}_last"
    if [ "$(cat $SRC | cksum | awk '{ printf $1 }')" = "${!val}" ]; then
        continue;
    fi
    if [ "${!val}" = "" ]; then
        echo -n export $val= >> .build.log
        cat $SRC | cksum | awk '{ print $1 }' >> .build.log
    else
        sed -i '' "s/${!val}/$(cat $SRC | cksum | awk '{ printf $1 }')/g" .build.log
    fi
    KERNEL_DIR_SRC_CHANGED=true
    OBJ="${BUILD_DIR}/${SRC%.c}.o"
    mkdir -p "$(dirname "$OBJ")"
    echo " -> $SRC"
    $CROSS_GCC -c "$SRC" -o "$OBJ" $CCFLAGS_KERNEL
done

find "${KERNEL_DIR}" -type f -name "*.asm" | while read -r SRC; do
    val1="${SRC//\//_}"
    val="${val1//./_}_last"
    if [ "$(cat $SRC | cksum | awk '{ printf $1 }')" = "${!val}" ]; then
        continue;
    fi
    if [ "${!val}" = "" ]; then
        echo -n export $val= >> .build.log
        cat $SRC | cksum | awk '{ print $1 }' >> .build.log
    else
        sed -i '' "s/${!val}/$(cat $SRC | cksum | awk '{ printf $1 }')/g" .build.log
    fi
    KERNEL_DIR_SRC_CHANGED=true
    OBJ="${BUILD_DIR}/${SRC%.asm}.o"
    mkdir -p "$(dirname "$OBJ")"
    echo " -> $SRC"
    $NASM $NASMFLAGS "$SRC" -o "$OBJ"
done

# Collect all object files
KERNEL_OBJECTS=$(find "$BUILD_DIR/$KERNEL_DIR" -type f -name "*.o")

# ───────────────────────────────────────────────
# LINK KERNEL
# ───────────────────────────────────────────────

if [ "$KERNEL_DIR_SRC_CHANGED" = true ]; then
    echo "[*] Linking kernel..."
    $LD -o "$KERNEL_IMG" $KERNEL_OBJECTS $LDFLAGS   
fi

# ───────────────────────────────────────────────
# ASSEMBLE BOOTSECTOR (make change in boot.asm to reflect)
# ───────────────────────────────────────────────
val1="${BOOTSECT_SRC//\//_}"
val="${val1//./_}_last"
if [ "$(cat $BOOTSECT_SRC | cksum | awk '{ printf $1 }')" = "${!val}" ]; then
    :
else
    echo "[*] Assembling bootsector..."
    $NASM -fbin "$BOOTSECT_SRC" -o "$BOOTSECT_BIN"
fi
if [ "${!val}" = "" ]; then
    echo -n export $val= >> .build.log
    cat $BOOTSECT_SRC | cksum | awk '{ print $1 }' >> .build.log
else
    sed -i '' "s/${!val}/$(cat $BOOTSECT_SRC | cksum | awk '{ printf $1 }')/g" .build.log
fi

# ───────────────────────────────────────────────
# ASSEMBLE BOOTLOADER (ABP)
# ───────────────────────────────────────────────

val1="${BOOTLOADER_SRC//\//_}"
val="${val1//./_}_last"
if [ "$(cat $BOOTLOADER_SRC | cksum | awk '{ printf $1 }')" = "${!val}" ]; then
    :
else
    echo "[*] Assembling bootloader..."
    # compile the BOOT.AEX file
    $NASM -fbin "$BOOTLOADER_SRC" -o "$BOOTLOADER_MAIN"
    BOOTLOADER_SRC_CHANGED=true
fi
if [ "${!val}" = "" ]; then
    echo -n export $val= >> .build.log
    cat $BOOTLOADER_SRC | cksum | awk '{ print $1 }' >> .build.log
else
    sed -i '' "s/${!val}/$(cat $BOOTLOADER_SRC | cksum | awk '{ printf $1 }')/g" .build.log
fi
# compile the LOADER.AEX file
BOOTLOADER_OBJECTS=""
# for each ASM file, compile it
for SRC in $BOOTLOADER_ASM_SRC; do
    OBJ="${BUILD_DIR}/${SRC%.asm}.o"
    BOOTLOADER_OBJECTS="$BOOTLOADER_OBJECTS $OBJ"

    val1="${SRC//\//_}"
    val="${val1//./_}_last"
    if [ "$(cat $SRC | cksum | awk '{ printf $1 }')" = "${!val}" ]; then
        continue;
    fi
    if [ "${!val}" = "" ]; then
        echo -n export $val= >> .build.log
        cat $SRC | cksum | awk '{ print $1 }' >> .build.log
    else
        sed -i '' "s/${!val}/$(cat $SRC | cksum | awk '{ printf $1 }')/g" .build.log
    fi

    BOOTLOADER_SRC_CHANGED=true
    mkdir -p "$(dirname "$OBJ")"
    echo " -> $SRC"
    $NASM $NASMFLAGS "$SRC" -o "$OBJ"
done
# for each C file, compile it
for SRC in $BOOTLOADER_C_SRC; do
    val1="${SRC//\//_}"
    val="${val1//./_}_last"
    if [ "$(cat $SRC | cksum | awk '{ printf $1 }')" = "${!val}" ]; then
        continue;
    fi
    if [ "${!val}" = "" ]; then
        echo -n export $val= >> .build.log
        cat $SRC | cksum | awk '{ print $1 }' >> .build.log
    else
        sed -i '' "s/${!val}/$(cat $SRC | cksum | awk '{ printf $1 }')/g" .build.log
    fi
    
    BOOTLOADER_SRC_CHANGED=true
    OBJ="${BUILD_DIR}/${SRC%.c}.o"
    BOOTLOADER_OBJECTS="$BOOTLOADER_OBJECTS $OBJ"
    mkdir -p "$(dirname "$OBJ")"
    echo " -> $SRC"
    $CROSS_GCC -c "$SRC" -o "$OBJ" $CCFLAGS_KERNEL
done
# link the bootloader
if [ "$BOOTLOADER_SRC_CHANGED" = true ]; then
    $LD -o "$BOOTLOADER_CASM" $BOOTLOADER_OBJECTS -m elf_i386 -Ttext 0x8000 --oformat binary
    $MKABP "$BOOTLOADER_COMP_DIR" "$BOOTLOADER_BIN"
    echo
fi

# ───────────────────────────────────────────────
# CREATE ISO IMAGE
# ───────────────────────────────────────────────

if [ "$KERNEL_DIR_SRC_CHANGED" = true ] || [ "$BOOTSECTOR_SRC_CHANGED" = true ] || [ "$BOOTLOADER_SRC_CHANGED" = true ]; then
    echo "[*] Creating bootable ISO image..."
    $DD if=/dev/zero of="$ISO_IMG" bs=512 count=1
    $DD if="$BOOTSECT_BIN" of="$ISO_IMG" conv=notrunc bs=512 seek=0 count=1
    $DD if="$BOOTLOADER_BIN" of="$ISO_IMG" conv=notrunc seek=1 bs=512
    SEEK=$(($(($(wc -c < $ISO_IMG)))512))
    echo BOOTLOADER SIZE: $(($SEEK - 1))
    $DD if='/Users/3024492/Downloads/disk.img' of="$ISO_IMG" conv=notrunc bs=512 seek=$SEEK
    # $DD if="$KERNEL_IMG" of="$ISO_IMG" conv=notrunc bs=512 seek=$SEEK

    $MV "$ISO_IMG" "$FINAL_ISO"
fi

echo "[✔] ISO built at: $FINAL_ISO"

# ───────────────────────────────────────────────
# (OPTIONAL) RUN IN QEMU
# ───────────────────────────────────────────────

if [[ "$1" == "run" ]]; then
    echo "[*] Launching QEMU..."
    $QEMU $QEMUFLAGS -hda "$FINAL_ISO"
fi
