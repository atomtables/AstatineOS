export default {
    source: ["./src"],
    build: "./build",
    directories: {
        product: "./product"
    },
    tools: {
        "i686-elf-gcc": {
            testCommand() { return `${this.path} --version`; }
        },
        "i686-elf-ld": {
            testCommand() { return `${this.path} --version`; }
        },
        nasm: {
            testCommand() { return `${this.path} --version`; }
        },
        dd: {
            testCommand() { return `${this.path} bs=512 count=1 if=/dev/zero of=./file && rm ./file`; }
        },
        "qemu-system-i386": {
            testCommand() { return `${this.path} --version`; }
        },
        mkabp: {
            testCommand() { return `${this.path} -v`; }
        },
        mformat: {
            testCommand() { return `${this.path} --version`; }
        },
        mcopy: {
            testCommand() { return `${this.path} --version`; }
        },
        "i686-astatine-gcc": {
            testCommand() { return `${this.path} -v`; }
        },
        "i686-astatine-ld": {
            testCommand() { return `${this.path} -v`; }
        }
    },
    paths: {
        "cross-include": {
            name: "include",
            find: [
                "i686-astatine-gcc --print-search-dirs",
                /^install: (.*\/lib\/.*)/
            ]
        },
        "cross-include-fixed": {
            name: "include-fixed",
            find: [
                "i686-astatine-gcc --print-search-dirs",
                /^install: (.*\/lib\/.*)/
            ]
        }
    },
    // these are referred to as "install" in individual build scripts
    // which helps to bootstrap them all into one.
    // those invididual items are built individually, then
    // compiled all together.
    // If there's no special needs, this is just to move them
    // to the correct output.
    // This can also be omitted (TODO)
    // since you write this file, you can just
    // hardcode the output
    locations: [
        {
            name: "bootsector",
            output: "$BUILD/boot.bin",
            // files will be in the form of how you set them for that given target
            build(files) {
                return [
                    `mv ${files[0]} $BUILD/boot.bin`
                ];
            }
        },
        {
            name: "bootloader",
            output: "$BUILD/abp.bin",
            require: {
                tools: {"mkabp": "MKABP"}
            },
            build(files) {
                let steps = [];
                for (let [location, copyTo] of files) {
                    steps.push(`mkdir -p $BUILD/bootloader_partition/${copyTo.split("/").slice(0, -1).join("/")}`);
                    steps.push(`cp ${location} $BUILD/bootloader_partition/${copyTo}`);
                }
                steps.push(`$MKABP $BUILD/bootloader_partition $BUILD/abp.bin`);
                return steps;
            }
        },
        {
            name: "fat32",
            output: "$BUILD/fat32.img",
            require: {
                tools: {"mformat": "MFORMAT", "mcopy": "MCOPY", "dd": "DD"}
            },
            build(files) {
                let steps = [];
                steps.push(`$DD bs=67M count=1 if=/dev/zero of=$BUILD/fat32.img`);
                steps.push(`$MFORMAT -F -i $BUILD/fat32.img ::`);
                for (let [location, copyTo] of files) {
                    steps.push(`$MCOPY -i $BUILD/fat32.img ${location} ::/${copyTo}`);
                }
                return steps;
            }
        }
    ],
    // targets then refer to building everything from these bootstrapped install target locations
    // they can also just be the only thing built, it works on dependencies
    // you hardcode the paths to the output.
    targets: [
        {
            name: "image",
            output: "$PRODUCT/AstatineOS.img",
            default: true,
            require: {
                directories: {"product": "PRODUCT"},
                tools: {"dd": "DD"}
            },
            prerequisites: [[
                // get size of abp.bin for seek calculation
                "echo $(($(($(wc -c < $BUILD/abp.bin)))/512+1))", "BOOTLOADER_FILE"
            ]],
            depends: ["bootsector", "bootloader", "fat32"],
            build: [
                "mkdir -p $PRODUCT",
                "$DD bs=512 count=1 if=$BUILD/boot.bin of=$PRODUCT/AstatineOS.img conv=notrunc",
                "$DD bs=512 if=$BUILD/abp.bin of=$PRODUCT/AstatineOS.img seek=1 conv=notrunc",
                "$DD bs=512 if=$BUILD/fat32.img of=$PRODUCT/AstatineOS.img seek=$BOOTLOADER_FILE conv=notrunc"
            ]
        }
    ],
    debug: [
        {
            name: "qemu",
            require: {
                tools: {
                    "qemu-system-i386": "QEMU"
                },
                directories: {"product": "PRODUCT"}
            },
            depends: ["image"],
            run: [
                "$QEMU -monitor stdio -d int,in_asm -D qemu.log -audiodev coreaudio,id=audio0 -machine pcspk-audiodev=audio0 -debugcon file:qemu.log -no-reboot -no-shutdown -drive file=\"$PRODUCT/AstatineOS.img\",format=raw,index=0,media=disk -m 512M"
            ]
        }
    ]
}