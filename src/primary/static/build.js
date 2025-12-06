export default {
    atomtools: true,
    output: ["generalfile", "musictrack"],
    install: {
        fat32: [[
            "$DIR/generalfile", "generalfile"
        ], [
            "$DIR/musictrack", "musictrack"
        ]]
    }
}