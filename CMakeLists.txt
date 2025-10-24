cmake_minimum_required(VERSION 3.28.1)

# below code is to make sure the compiler/asmbler actually exists
set(CMAKE_C_COMPILER i686-elf-gcc)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_C_FLAGS "-O0 -ggdb")
set(CMAKE_ASM_COMPILER nasm)
set_property(DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES ./**/*.bin ./**/*.o ./**/*.dis ./**/*.iso ./**/*.log ./**/*.map ./*.o ./*.bin ./*.dis ./*.iso ./*.log ./*.map build product)
# Ring 0 Code, or the Kernel, will be in the kernel directory
project(NetworkOS LANGUAGES C ASM)

# required directories for compilation
set(SOURCE_DIR "src")
file(MAKE_DIRECTORY ${SOURCE_DIR})
set(BUILD_DIR "objects")
file(MAKE_DIRECTORY ${BUILD_DIR})
set(BOOT_DIR "boot")
set(LOADER_DIR "stage2")
set(KERNEL_DIR "kernel")
set(LIBRARY_DIR "libraries")
file(MAKE_DIRECTORY "product")

# required executables
set(QEMU qemu-system-i386-unsigned)
set(GCC i686-elf-gcc)
set(LD i686-elf-ld)
set(CAT cat)
set(RM rm -rf)
set(NASM nasm)
set(DD dd)
set(MKDIR mkdir)
set(MV mv)

# required flags
set(NASMFLAGS -felf)
set(LDFLAGS
        -m elf_i386 -Ttext 0x10000 --oformat binary
        -L ${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/${LIBRARY_DIR} -lgcc
)
set(CCFLAGS_KERNEL
        -m32 -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
        -Wno-pointer-arith -Wno-unused-parameter
        -nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector
        -fno-builtin-function -fno-builtin -I${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/${KERNEL_DIR}
        -I/opt/homebrew/Cellar/i686-elf-gcc/15.1.0/lib/gcc/i686-elf/15.1.0/include
        -I/opt/homebrew/Cellar/i686-elf-gcc/15.1.0/lib/gcc/i686-elf/15.1.0/include-fixed
)
set(QEMUFLAGS -monitor stdio -d int -D qemu.log -audiodev coreaudio,id=audio0 -machine pcspk-audiodev=audio0 -debugcon file:qemu.log) # -no-reboot -no-shutdown)

# get all sources
file(GLOB_RECURSE KERNEL_HEADERS "${SOURCE_DIR}/${KERNEL_DIR}/*.h")
file(GLOB_RECURSE KERNEL_SOURCES_C "${SOURCE_DIR}/${KERNEL_DIR}/*.c")
file(GLOB_RECURSE KERNEL_SOURCES_ASM "${SOURCE_DIR}/${KERNEL_DIR}/*.asm")
set(BOOTSECT_SOURCES "${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/${BOOT_DIR}/boot.asm")
set(BOOTLOAD_SOURCES "${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/${LOADER_DIR}/vbr.asm")

# process the names of compiled files and dirctories
set(KERNEL_OBJECTS "")
foreach (source ${KERNEL_SOURCES_C})
    string(REPLACE "${SOURCE_DIR}/" "${BUILD_DIR}/" object ${source})
    string(REPLACE ".c" ".o" object ${object})
    list(APPEND KERNEL_OBJECTS ${object})
endforeach ()
foreach (source ${KERNEL_SOURCES_ASM})
    string(REPLACE "${SOURCE_DIR}/" "${BUILD_DIR}/" object ${source})
    string(REPLACE ".asm" ".o" object ${object})
    list(APPEND KERNEL_OBJECTS ${object})
endforeach ()
set(BUILD_DIRS "")
foreach (object ${KERNEL_OBJECTS})
    get_filename_component(DIR ${object} DIRECTORY)
    list(APPEND BUILD_DIRS ${DIR})
endforeach ()
list(REMOVE_DUPLICATES BUILD_DIRS)
list(APPEND BUILD_DIRS "product")

# Paths of completed files
set(BOOTSECT "${PROJECT_SOURCE_DIR}/${BUILD_DIR}/boot.bin")
set(BOOTLOAD "${PROJECT_SOURCE_DIR}/${BUILD_DIR}/loader.bin")
set(KERNEL "${PROJECT_SOURCE_DIR}/${BUILD_DIR}/kernel.img")
set(ISO "${PROJECT_SOURCE_DIR}/${BUILD_DIR}/NetworkOS.iso")
set(FISO "${PROJECT_SOURCE_DIR}/product/NetworkOS.iso")

# compilation rules
foreach (SRC_FILE ${KERNEL_SOURCES_ASM})
    string(REPLACE ".asm" ".o" OBJECT_FILE ${SRC_FILE})
    string(REPLACE "${SOURCE_DIR}" "${BUILD_DIR}" OBJECT_FILE ${OBJECT_FILE})
    get_filename_component(OBJECT_FILE_DIR ${OBJECT_FILE} DIRECTORY)

    # Create the object file directories
    file(MAKE_DIRECTORY ${OBJECT_FILE_DIR})

    add_custom_command(
            OUTPUT ${OBJECT_FILE}
            COMMAND ${NASM} ${NASMFLAGS} ${SRC_FILE} -o ${OBJECT_FILE}
            DEPENDS ${SRC_FILE}
            COMMENT "Assembling ${SRC_FILE} to ${OBJECT_FILE}"
    )
endforeach ()
foreach (SRC_FILE ${KERNEL_SOURCES_C})
    string(REPLACE ".c" ".o" OBJECT_FILE ${SRC_FILE})
    string(REPLACE "${SOURCE_DIR}" "${BUILD_DIR}" OBJECT_FILE ${OBJECT_FILE})
    get_filename_component(OBJECT_FILE_DIR ${OBJECT_FILE} DIRECTORY)

    # Create the object file directories
    file(MAKE_DIRECTORY ${OBJECT_FILE_DIR})

    add_custom_command(
            OUTPUT ${OBJECT_FILE}
            COMMAND ${GCC} -c ${SRC_FILE} -o ${OBJECT_FILE} ${CCFLAGS_KERNEL}
            DEPENDS ${SRC_FILE}
            COMMENT "Compiling ${SRC_FILE} to ${OBJECT_FILE}"
    )
endforeach ()

add_custom_command(
        OUTPUT ${BOOTSECT}
        COMMAND ${NASM} -fbin ${BOOTSECT_SOURCES} -o ${BOOTSECT}
        DEPENDS ${BOOTSECT_SOURCES}
        COMMENT "Assembling bootsector ${BOOTSECT_SOURCES}"
)

add_custom_command(
        OUTPUT ${BOOTLOAD}
        COMMAND ${NASM} -fbin ${BOOTLOAD_SOURCES} -o ${BOOTLOAD}
        DEPENDS ${BOOTLOAD_SOURCES} ${BOOTLOAD_DEPS}
        COMMENT "Assembling MBR-only bootloader ${BOOTLOAD_SOURCES}"
)

add_custom_command(
        OUTPUT ${KERNEL}
        COMMAND ${LD} -o ${KERNEL} ${KERNEL_OBJECTS} ${LDFLAGS}
        DEPENDS ${KERNEL_OBJECTS} ${KERNEL_HEADERS}
        COMMENT "Linking kernel ${KERNEL}"
)

add_custom_command(
        OUTPUT ${FISO}
        COMMAND ${MKDIR} -p product
        COMMAND ${DD} if=/dev/zero of=${ISO} bs=512 count=5760
        COMMAND ${DD} if=${BOOTSECT} of=${ISO} conv=notrunc bs=512 seek=0 count=1
        COMMAND ${DD} if=${BOOTLOAD} of=${ISO} conv=notrunc bs=512 seek=1 count=2
        COMMAND ${DD} if=${KERNEL} of=${ISO} conv=notrunc bs=512 seek=3 count=5757
        COMMAND ${MV} ${ISO} ${FISO}
        DEPENDS ${BOOTSECT} ${BOOTLOAD} ${KERNEL}
        COMMENT "Creating bootable media ${ISO}"
)

add_custom_target(
        build
        DEPENDS ${FISO}
)

add_custom_target(
        run ALL
        COMMAND ${QEMU} ${QEMUFLAGS} -hda ${FISO}
        DEPENDS build
)

# for the discovery of files that I can apply for CLion
# the actual execution of this will fail because i686-elf-gcc is a cross-compiler for x86
# and im on an apple silicon mac. might work for prince though
add_executable(FailStub ${KERNEL_HEADERS} ${KERNEL_SOURCES_C} ${KERNEL_SOURCES_ASM} ${BOOTSECT_SOURCES} ${BOOTSECT_DEPS})
target_include_directories(FailStub PRIVATE "${SOURCE_DIR}/${KERNEL_DIR}" "/opt/homebrew/Cellar/i686-elf-gcc/14.1.0/lib/gcc/i686-elf/14.1.0/include" "/opt/homebrew/Cellar/i686-elf-gcc/14.1.0/lib/gcc/i686-elf/14.1.0/include-fixed")

# These are actual executables designed to run on the host system
set(CMAKE_C_COMPILER gcc)
set(UTILITIES_BASE_DIR "utilities")
set(MKABP_DIR "${UTILITIES_BASE_DIR}/mkabp")

file(GLOB_RECURSE MKABP_HEADERS "${MKABP_DIR}/*.h")
file(GLOB_RECURSE MKABP_SOURCES "${MKABP_DIR}/*.c")
add_executable(mkabp ${MKABP_HEADERS} ${MKABP_SOURCES})