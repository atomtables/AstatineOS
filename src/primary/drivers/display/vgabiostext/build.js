export default {
    atomtools: true,
    require: {
        tools: {
            "i686-elf-gcc": "GCC"
        }
    },
    build: [
        {
            type: "command",
            command: "$GCC $DIR/main.c -o $BUILD/textmode.adv -fPIC -pie -I '$PROJECT/include/kernel' " +
                     "-ffreestanding -nostdlib -nostdinc -fno-stack-protector -fno-builtin -m32 -std=gnu11 " +
                     "-Wl,-z,notext -Wl,-nostdlib -Wl,--omagic -Wl,--no-dynamic-linker"
        }
    ],
    output: ["textmode.adv"],
    install: {
        fat32: [[
            "$BUILD/textmode.adv", "drivers/textmode.adv"
        ]]
    }
}