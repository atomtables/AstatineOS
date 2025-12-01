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
            command: "$GCC $DIR/main.c -o $BUILD/textmode.adv -shared -fPIC -I '$PROJECT/include/kernel' " +
                     "-ffreestanding -nostdlib -nostdinc -fno-stack-protector -fno-builtin -m32 -std=gnu11"
        }
    ],
    output: ["textmode.adv"],
    install: {
        fat32: [[
            "$BUILD/textmode.adv", "textmode.adv"
        ]]
    }
}