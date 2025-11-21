export default {
    atomtools: true,
    require: {
        tools: {
            "i686-astatine-gcc": "HOSTED_GCC"
        }
    },
    build: [
        {
            type: "command",
            command: "$HOSTED_GCC -g -O0 $DIR/main.c -o $BUILD/atf.aex"
        }
    ],
    output: ["atf.aex"],
    install: {
        fat32: [[
            "$BUILD/atf.aex", "atf.aex"
        ]]
    }
}