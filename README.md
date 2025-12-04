# AstatineOS

Do `./build.sh` to compile, `./build.sh run` to compile and run. Before running, compile and install `mkabp` to PATH.

## Dependencies
- `nasm`
- `i686-elf` toolchain, including `gcc` and `ld`
- `i686-astatine` and `newlib-astatine` toolchain, see below
- `dd`
- `qemu` for use of `qemu-system-i386` or `qemu-system-x86_64`
- general gnu tools like `xxd` and `autotools`

## Compilation support
macos and linux only. when windows fixes their operating system i'll include support.
### Build System
uses my custom build system that's built in typescript and javascript. why? because i'd really like a completely modular build system. Cmake's good but its really restricted and doesn't work well for my needs. GNU-Make is also good but it's not nearly as modular (as far as I know I could have missed a feature) and I found that honestly a build.sh script worked better for me. Autotools probably works best with actual C executables but I haven't figured out the black magic required to get it working for this and honestly the JS system isn't THAT slow, it runs all the build commands in parallel when possible even if there's more overhead associated with running a build command compared to a .sh file.

## mkabp
`mkabp` is a utility to create an AstatineOS Boot partition using a folder. It will create a bootable partition image, where 
the bootstrap code, if ran, will attempt to run a BOOT.AEX file. 

`mkabp` uses the Autotools system to compile. Install autotools and run `autoreconf -i`, then do the `./configure && make && make install` to add to path. You must install `mkabp` to path in order to compile AstatineOS.

There's also a package available that you can just `./configure && make && make install` in releases

## Toolchain and libc
I don't feel like writing a compiler by myself (obviously) so there is an i686-gcc toolchain for AstatineOS and also a i686-libc courtesy of newlib.
The specific versions that include the changes I made are below.
- i686-astatine-binutils: (https://github.com/atomtables/i686-astatine-binutils)[https://github.com/atomtables/i686-astatine-binutils]
- i686-astatine-gcc: (https://github.com/atomtables/i686-astatine-gcc)[https://github.com/atomtables/i686-astatine-gcc]
- newlib-astatine: (https://github.com/atomtables/newlib_astatine)[https://github.com/atomtables/newlib_astatine]

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
- [X] paging
- [X] virtual memory
- [ ] long mode (64-bit)
- [X] libraries (newlib)
- [x] custom boot filesystem for loading boot image (instead of being sane and pulling a grub)
- [x] os-specific toolchain for building apps
- [x] libc or any functions to run apps that don't involve building them yourselves
- [ ] sound support (technically exists but 1-bit sound is kinda sad even for 1995)
- [-] modular driver support
- [ ] documentation
- [x] custom build system (hey if it works it works)
- [x] kernelmode code execution
- [X] usermode applications (technically)
- [ ] tele-type-writer mode (graphical text interface) (lowk the priority now)

## goals
As far as (literally everyone) says, 32-bit is mostly doomed and you should go for a 64-bit kernel. For the
most part, I think after a good development of usermode applications, it would be a good time to transition
the kernel to 64-bit MBR. It would remove the added complexity of segmentation while providing infinitely
more space for virtual addressing. Before going for the added complexity it's probably still better to learn
how to do it simply.

64-bit doesn't look too hard anyway, the bootloader and boot.aex would def change but the rest of the kernel 
doesn't significantly look like it would need a full rewrite. I guess some additions like more modern hardware
(remember our definition of modern is 2003) like the HPET and the APIC would be in order before a full
conversion. It's not like I'm switching to arm anyways. It would also be pretty likely that some of the files 
would be able to interconnect, so you could have an i686 build and a x86_64 build together.
That's for WAY later me though.

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

## it works on fake hardware bro trust me
source: this video
<video width="630" height="300" src="https://github.com/user-attachments/assets/9c548362-ebc2-4719-b523-f6a34fe98653"></video>

[https://github.com/user-attachments/assets/9c548362-ebc2-4719-b523-f6a34fe98653
](https://github.com/user-attachments/assets/9c548362-ebc2-4719-b523-f6a34fe98653
)









Thanks and have a nice day !!!!!!
