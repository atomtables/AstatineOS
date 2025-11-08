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

## current features (more planned these are just the ones i can think of)
- [x] bootloader
- [x] splash screen
- [ ] graphics
   - [ ] graphical user interface
- [ ] 64-bit mode
- [x] filesystem
- [ ] calculator app (wwdc2024)
- [ ] usb stack
- [ ] any sort of connectivity that doesn't involve a PATA hard drive in the first channel's master
- [ ] paging
- [ ] virtual memory
- [ ] libraries
- [x] custom boot filesystem for loading boot image (instead of being sane and pulling a grub)
- [ ] os-specific toolchain for building apps
- [ ] libc or any functions to run apps that don't involve building them yourselves
- [ ] sound support (technically exists but 1-bit sound is kinda sad even for 1995)
- [ ] modular driver support
- [ ] documentation
- [x] custom build system (hey if it works it works)
- [x] kernelmode code execution
- [ ] usermode applications

## Why?
Web development, app development, development for a specific platform is fun and all. But at some point, you
want to spice things up. Instead of developing on top of dependencies, you want to try to be the dependency, to 
be the brains behind the operation. I am making this project to learn about the different components that make
up our modern operating systems, as OSes can't just say "You're holding it wrong" or "You have incompatible 
hardware". If something goes wrong, it's up to the OS to make sure it doesn't crash the entire computer, and 
learning about that responsibility and how to live up to it is really really cool.

## How? (can you demo this yourself)?
Build it yourself using all the dependencies above. 

Or, download an ISO from the releases section below and test it out yourself on

 [https://copy.sh/v86] (works 100% with latest release)

## it works on real hardware bro trust me
source: this video
<video width="630" height="300" src="https://github.com/user-attachments/assets/2a29ba96-8f2c-4b28-846f-bb0438e4ea46"></video>
[https://github.com/user-attachments/assets/2a29ba96-8f2c-4b28-846f-bb0438e4ea46](https://github.com/user-attachments/assets/2a29ba96-8f2c-4b28-846f-bb0438e4ea46)

Thanks and have a nice day !!!!!!
