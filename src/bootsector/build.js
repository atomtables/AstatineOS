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
            command: "$NASM -fbin $DIR/boot.asm -o $BUILD/boot.bin",
        }
    ],
    // the output is always relative to $BUILD
    output: ["boot.bin"],
    install: {
        bootsector: "$BUILD/boot.bin"
    }
}