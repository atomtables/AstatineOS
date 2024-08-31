# NetworkOS

Use `cmake ..` in a build folder. then do `make build` to build an iso, or `make run` to run it in `qemu-system-i386`

## Dependencies
- `nasm`
- `i686-elf` toolchain, including `gcc` and `ld`
- `dd` (unless on windows in which case modify the CMakeLists.txt)
- `qemu` for use of `qemu-system-i386` or `qemu-system-x86_64`

## Why?
We wanted to make a simplistic Networking Operating System for fun and for expandability. Kind of like a TempleOS.