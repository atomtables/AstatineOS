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
            command: "$GCC $DIR/main.c -o $BUILD/textmode.adv -I '$PROJECT/include/kernel' -Wl,-Ttext=0x07000000,-Tdata=0x06000000 " +
                     "-ffreestanding -nostdlib -nostdinc -fno-pie -fno-stack-protector -fno-builtin -m32 -std=gnu11"
        }
    ],
    output: ["textmode.adv"],
    install: {
        fat32: [[
            "$BUILD/textmode.adv", "textmode.adv"
        ]]
    }
}