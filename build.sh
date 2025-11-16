#!/usr/bin/env bash
# got GPT to convert the cmakelists.txt to this because cmake sucks for custom comp
# uses .build.log for saving previous hashes
set -e

# ───────────────────────────────────────────────
# CONFIGURATION
# ───────────────────────────────────────────────

    CROSS_GCC=i686-elf-gcc
    LD=i686-elf-ld
    NASM=nasm
    DD=dd
    MV=mv
    MKDIR=mkdir
    QEMU=qemu-system-i386
    MKABP=mkabp
    XXD=xxd

    MFORMAT=mformat
    MCOPY=mcopy

    # TODO: MUST BE CHANGED FOR LOCAL COMPILATION
    CROSS_GCC_INCLUDE=/opt/homebrew/Cellar/i686-elf-gcc/15.2.0/lib/gcc/i686-elf/15.2.0/include
    CROSS_GCC_INCLUDE_FIXED=/opt/homebrew/Cellar/i686-elf-gcc/15.2.0/lib/gcc/i686-elf/15.2.0/include-fixed

# ───────────────────────────────────────────────
# DIRECTORY STRUCTURE
# ───────────────────────────────────────────────

    SOURCE_DIR="src"
    BUILD_DIR="objects"
    BOOT_DIR="${SOURCE_DIR}/bootsector"
    BOOTLOADER_DIR="${SOURCE_DIR}/bootloader_partition"

    FAT32_SOURCES_DIR="${SOURCE_DIR}/primary"

    KERNEL_RESOURCES="${FAT32_SOURCES_DIR}/static"
    KERNEL_DIR="${FAT32_SOURCES_DIR}/kernel"
    LIBRARY_DIR="libraries"
    PRODUCT_DIR="product"
    
    BOOTLOADER_COMP_DIR="${BUILD_DIR}/ABP"

    mkdir -p "$BUILD_DIR" "$BOOTLOADER_COMP_DIR" "$PRODUCT_DIR"

# ───────────────────────────────────────────────
# FLAGS
# ───────────────────────────────────────────────

    NASMFLAGS="-felf"
    LDFLAGS="-m elf_i386 -Ttext 0x10000 --oformat binary -Map kernel.map"
    CCFLAGS_KERNEL="-m32 -std=gnu11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
    -Wno-pointer-arith -Wno-unused-parameter -fno-delete-null-pointer-checks \
    -nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector \
    -fno-builtin-function -fno-builtin
    -I${KERNEL_DIR} -I${CROSS_GCC_INCLUDE} -I${CROSS_GCC_INCLUDE_FIXED}"

    QEMUFLAGS="-monitor stdio -d int,in_asm -D qemu.log -audiodev coreaudio,id=audio0 -machine pcspk-audiodev=audio0 -debugcon file:qemu.log -no-reboot -no-shutdown"

# ───────────────────────────────────────────────
# PATHS
# ───────────────────────────────────────────────

    BOOTSECT_SRC="${BOOT_DIR}/boot.asm"
    BOOTSECT_BIN="${BUILD_DIR}/boot.bin"

    BOOTLOADER_boot_aex_src="${BOOTLOADER_DIR}/boot.aex/boot.aex.asm"
    BOOTLOADER_loader_aex_SRC="${BOOTLOADER_DIR}/loader.aex"
    BOOTLOADER_BIN="${BUILD_DIR}/abp.bin"
    BOOTLOADER_boot_aex="${BOOTLOADER_COMP_DIR}/BOOT.AEX"
    BOOTLOADER_loader_aex="${BOOTLOADER_COMP_DIR}/LOADER.AEX"

    KERNEL_IMG="${BUILD_DIR}/kernel.bin"
    FAT32_PART_IMG="${BUILD_DIR}/fat32.img"
    FAT32_PART_SIZE="66M"

    ISO_IMG="${BUILD_DIR}/NetworkOS.img"
    FINAL_ISO="${PRODUCT_DIR}/NetworkOS.img"
# ───────────────────────────────────────────────
# CHANGE SWITCHES
# ───────────────────────────────────────────────
    KERNEL_DIR_SRC_CHANGED=false
    BOOTSECTOR_SRC_CHANGED=false
    BOOTLOADER_SRC_CHANGED=false


# ───────────────────────────────────────────────
# Switches
# ───────────────────────────────────────────────

NO_BUILD=false
RUN=false
CLEAN=false
WITH_GDB=
for arg in "$@"; do
  if [ $arg = "--no-build" ]; then
      NO_BUILD=true
  fi
  if [ $arg = "run" ]; then
      RUN=true
  fi
  if [ $arg = "clean" ]; then
      CLEAN=true
  fi
  if [ $arg = "--with-gdb" ]; then
      WITH_GDB="-s -S"
  fi
done

# ───────────────────────────────────────────────
# CLEAN BUILD
# ───────────────────────────────────────────────
if [ "$CLEAN" = true ]; then
    echo "[*] Cleaning build directory..."
    rm -rf "$BUILD_DIR" "$PRODUCT_DIR"
    exit 0
fi

# ───────────────────────────────────────────────
# BUILD PROCESS
# ───────────────────────────────────────────────

if [ "$NO_BUILD" = false ]; then
    if [ -e ".build.log" ]; then
    #   source .build.log
        :
    else
        echo "Running a clean compilation."
    fi

    # ───────────────────────────────────────────────
    # only rebuild if file changed
    # ───────────────────────────────────────────────

    file_changed() {
        local SRC="$1"
        local val1="${SRC//\//_}"
        local val="${val1//./_}_last"
        local current_checksum=$(cat "$SRC" | cksum | awk '{ printf $1 }')
        
        if [ "$current_checksum" = "${!val}" ]; then
            return 1  # unchanged
        fi
        return 0  # changed
    }
    update_checksum() {
        local SRC="$1"
        local val1="${SRC//\//_}"
        local val="${val1//./_}_last"
        local current_checksum=$(cat "$SRC" | cksum | awk '{ printf $1 }')
        
        if [ "${!val}" = "" ]; then
            echo -n "export $val=" >> .build.log
            echo "$current_checksum" >> .build.log
        else
            sed -i '' "s/${!val}/${current_checksum}/g" .build.log
        fi
    }
    # ───────────────────────────────────────────────
    # COMPILE KERNEL (C + ASM)
    # ───────────────────────────────────────────────

    echo "[*] Compiling kernel sources..."
    find "${KERNEL_DIR}" -type f -name "*.c" | while read -r SRC; do
        if ! file_changed "$SRC"; then
            continue
        fi
        KERNEL_DIR_SRC_CHANGED=true
        OBJ="${BUILD_DIR}/${SRC%.c}.o"
        mkdir -p "$(dirname "$OBJ")"
        echo " -> $SRC"
        $CROSS_GCC -c "$SRC" -o "$OBJ" $CCFLAGS_KERNEL
        update_checksum "$SRC"
    done

    find "${KERNEL_DIR}" -type f -name "*.asm" | while read -r SRC; do
        if ! file_changed "$SRC"; then
            continue
        fi
        KERNEL_DIR_SRC_CHANGED=true
        OBJ="${BUILD_DIR}/${SRC%.asm}.o"
        mkdir -p "$(dirname "$OBJ")"
        echo " -> $SRC"
        $NASM $NASMFLAGS "$SRC" -o "$OBJ"
        update_checksum "$SRC"
    done

    # Collect all object files
    KERNEL_OBJECTS=$(find "$BUILD_DIR/$KERNEL_DIR" -type f -name "*.o")

    # ───────────────────────────────────────────────
    # LINK KERNEL
    # ───────────────────────────────────────────────

    echo "[*] Linking kernel..."
    $LD -o "$KERNEL_IMG" $KERNEL_OBJECTS $LDFLAGS  

    # ───────────────────────────────────────────────
    # FORMAT FAT32 PARTITION
    # ───────────────────────────────────────────────
    echo "[*] Creating FAT32 partition..."
    if [ -e "$FAT32_PART_IMG" ]; then
        rm "$FAT32_PART_IMG"
    fi
    $DD if=/dev/zero of="$FAT32_PART_IMG" bs=$FAT32_PART_SIZE count=1
    $MFORMAT -F -i "$FAT32_PART_IMG" ::
    $MCOPY -i "$FAT32_PART_IMG" "$KERNEL_IMG" ::
    $MCOPY -i "$FAT32_PART_IMG" $KERNEL_RESOURCES/* ::

    # ───────────────────────────────────────────────
    # ASSEMBLE BOOTSECTOR (make change in boot.asm to reflect)
    # ───────────────────────────────────────────────
    if file_changed "$BOOTSECT_SRC"; then
        echo "[*] Assembling bootsector..."
        $NASM -fbin "$BOOTSECT_SRC" -o "$BOOTSECT_BIN"
        BOOTSECTOR_SRC_CHANGED=true
        update_checksum "$BOOTSECT_SRC"
    fi

    # ───────────────────────────────────────────────
    # ASSEMBLE BOOTLOADER (ABP)
    # ───────────────────────────────────────────────

    if file_changed "$BOOTLOADER_boot_aex_src"; then
        echo "[*] Assembling bootloader..."
        # compile the BOOT.AEX file
        $NASM -fbin "$BOOTLOADER_boot_aex_src" -o "$BOOTLOADER_boot_aex"
        BOOTLOADER_SRC_CHANGED=true
        update_checksum "$BOOTLOADER_boot_aex_src"
    fi

    # compile the LOADER.AEX file
    BOOTLOADER_OBJECTS=""
    # for each ASM file, compile it
    find "${BOOTLOADER_loader_aex_SRC}" -type f -name "*.asm" | while read -r SRC; do
        OBJ="${BUILD_DIR}/${SRC%.asm}.o"

        if ! file_changed "$SRC"; then
            continue
        fi
        BOOTLOADER_SRC_CHANGED=true

        mkdir -p "$(dirname "$OBJ")"
        echo " -> $SRC"
        $NASM $NASMFLAGS "$SRC" -o "$OBJ"
        update_checksum "$SRC"
    done
    # for each C file, compile it
    find "${BOOTLOADER_loader_aex_SRC}" -type f -name "*.c" | while read -r SRC; do
        OBJ="${BUILD_DIR}/${SRC%.c}.o"

        if ! file_changed "$SRC"; then
            continue
        fi
        BOOTLOADER_SRC_CHANGED=true

        mkdir -p "$(dirname "$OBJ")"
        echo " -> $SRC"
        $CROSS_GCC -c "$SRC" -o "$OBJ" $CCFLAGS_KERNEL
        update_checksum "$SRC"
    done
    BOOTLOADER_loader_aex_OBJECTS=$(find "$BUILD_DIR/$BOOTLOADER_loader_aex_SRC" -type f -name "*.o")
    # link the bootloader
    if [ "$BOOTLOADER_SRC_CHANGED" = true ]; then
        echo $BOOTLOADER_loader_aex_OBJECTS
        $LD -o "$BOOTLOADER_loader_aex" $BOOTLOADER_loader_aex_OBJECTS -m elf_i386 -Ttext 0x8000 --oformat binary
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
        SEEK=$(($(($(wc -c < $ISO_IMG)))/512))
        echo BOOTLOADER SIZE: $(($SEEK - 1))
        $DD if="$FAT32_PART_IMG" of="$ISO_IMG" conv=notrunc bs=512 seek=$SEEK
        # $DD if="$KERNEL_IMG" of="$ISO_IMG" conv=notrunc bs=512 seek=$SEEK

        $MV "$ISO_IMG" "$FINAL_ISO"
    fi

    echo "[✔] ISO built at: $FINAL_ISO"
fi

# ───────────────────────────────────────────────
# RUN IN QEMU
# ───────────────────────────────────────────────

if [ "$RUN" = true ]; then
    echo "[*] Launching QEMU..."
    $QEMU $QEMUFLAGS -drive file="$FINAL_ISO,format=raw,index=0,media=disk" $WITH_GDB
fi
