export default {
    atomtools: true,
    require: {
        tools: {
            "nasm": "NASM"
        }
    },
    build: [
        {
            type: "command",
            command: "$NASM -fbin $DIR/boot.aex.asm -o $BUILD/BOOT.AEX",
        }
    ],
    // the output is always relative to $BUILD
    output: ["BOOT.AEX"],
    install: {
        bootloader: [[
            "$BUILD/BOOT.AEX", "BOOT.AEX"
        ]]
    }
}