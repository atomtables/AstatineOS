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
            command: "$HOSTED_GCC -g -O0 -o $BUILD/basicbasic.aex $DIR/basicbasic.c -I $PROJECT/include/user"
        }
    ],
    output: ["basicbasic.aex"],
    install: {
        fat32: [[
            "$BUILD/basicbasic.aex", "basicbasic.aex"
        ]]
    }
}