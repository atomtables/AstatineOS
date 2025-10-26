# AstatineOS

Do `./build.sh` to compile, `./build.sh run` to compile and run. Before running, compile and install `mkabp` to PATH.

## Dependencies
- `nasm`
- `i686-elf` toolchain, including `gcc` and `ld`
- `dd`
- `qemu` for use of `qemu-system-i386` or `qemu-system-x86_64`
- general gnu tools like `xxd` and `autotools`

## Compilation support
macos and linux only. when windows fixes their operating system i'll include support.

## mkabp
`mkabp` is a utility to create an AstatineOS Boot partition using a folder. It will create a bootable partition image, where 
the bootstrap code, if ran, will attempt to run a BOOT.AEX file. 

`mkabp` uses the Autotools system to compile. Install autotools and run `autoreconf -i`, then do the `./configure && make && make install` to add to path. You must install `mkabp` to path in order to compile AstatineOS.

There's also a package available that you can just `./configure && make && make install` in releases

## Why?
Web development, app development, development for a specific platform is fun and all. But at some point, you
want to spice things up. Instead of developing on top of dependencies, you want to try to be the dependency, to 
be the brains behind the operation. I am making this project to learn about the different components that make
up our modern operating systems, as OSes can't just say "You're holding it wrong" or "You have incompatible 
hardware". If something goes wrong, it's up to the OS to make sure it doesn't crash the entire computer, and 
learning about that responsibility and how to live up to it is really really cool.

## How? (can you demo this yourself)?
Build it yourself using all the dependencies above. 

Or, download an ISO from the releases section below and test it out yourself on [https://copy.sh/v86].

## it works on real hardware bro trust me
source: this video
![https://github.com/user-attachments/assets/6a99789c-01c5-4923-ba64-a16864090a6a](https://github.com/user-attachments/assets/6a99789c-01c5-4923-ba64-a16864090a6a)

Thanks and have a nice day !!!!!!
