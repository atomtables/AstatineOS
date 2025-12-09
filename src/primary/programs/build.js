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
            command: "$HOSTED_GCC -g -O0 -o $BUILD/atf.aex $DIR/main.c -I $PROJECT/include/user"
        }
    ],
    output: ["atf.aex"],
    install: {
        fat32: [[
            "$BUILD/atf.aex", "atf.aex"
        ]]
    }
}