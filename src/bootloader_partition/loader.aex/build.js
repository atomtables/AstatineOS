export default {
    atomtools: true,
    require: {
        tools: {
            "nasm": "NASM",
            "i686-elf-gcc": "GCC",
            "i686-elf-ld": "LD"
        },
        paths: {
            "cross-include": "CROSS_GCC_INCLUDE",
            "cross-include-fixed": "CROSS_GCC_INCLUDE_FIXED"
        }
    },
    build: [
        {
            type: "foreach",
            input: "$DIR/**/*.asm",
            output: "$BUILD/$FILE_BASE.o",
            build: [
                {
                    type: "command",
                    command: "$NASM -felf $INPUT -o $OUTPUT"
                }
            ]
        },
        {
            type: "foreach",
            input: "$DIR/**/*.c",
            output: "$BUILD/$FILE_BASE.o",
            build: [
                {
                    type: "command",
                    command: "$GCC -c $INPUT -o $OUTPUT -m32 -std=gnu11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing " +
                                "-Wno-pointer-arith -Wno-unused-parameter -fno-delete-null-pointer-checks " +
                                "-nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector " +
                                "-fno-builtin-function -fno-builtin " +
                                "-I$CROSS_GCC_INCLUDE -I$CROSS_GCC_INCLUDE_FIXED"
                }
            ]   
        },
        {
            type: "command",
            prerequisites: [[
                "find $BUILD/$DIR -name '*.o' -print0 | xargs -0 echo", "ALL_OBJECTS"
            ]],
            command: "$LD -m elf_i386 -Ttext 0x8000 --oformat binary -o $BUILD/LOADER.AEX $ALL_OBJECTS"
        }
    ],
    // the output is always relative to $BUILD
    output: ["LOADER.AEX"],
    install: {
        bootloader: [[
            "$BUILD/LOADER.AEX", "LOADER.AEX"
        ]]
    }
}