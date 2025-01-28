# NetworkOS

Use `cmake ..` in a build folder. then do `make build` to build an iso, or `make run` to run it in `qemu-system-i386`

## Dependencies
- `nasm`
- `i686-elf` toolchain, including `gcc` and `ld`
- `dd` (unless on windows in which case modify the CMakeLists.txt)
- `qemu` for use of `qemu-system-i386` or `qemu-system-x86_64`

## Why?
I wanted to make a simplistic Networking Operating System for fun and for expandability. Kind of like a TempleOS. 
Learning operating system development is extremely interesting, and learning the different aspects of what makes a
program tick, from the function call to the internal assembly. Understanding the design decisions that people in the
90s made (and understanding why they're still being used) gives context into what makes a program live long and prosper.
Also, it's fun being challenged to create code that has an extremely high chance of randomly breaking. (Also to add that
a bug could be caused by anything from reading an incorrect number of sectors to the C compiler putting a ud2 instruction.)

## How? (can you demo this yourself)?
Build it yourself using all the dependencies above. Keep in mind that you will need to make significant changes to make the
build file work on Windows or Linux (but on macOS it works).

Or, download an ISO from the releases section below and test it out yourself on (https://copy.sh/v86)[https://copy.sh/v86].

Thanks and have a nice day !!!!!!
