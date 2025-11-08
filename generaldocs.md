# general docs

## memory maps
- keeping the lower 64kb of memory free in case i ever need to launch v86 tasks
    - realistically there's no reason to launch a v86 task but in case it's better to have it?
- memory maps will mostly come into play once paging and VMM is implemented.

## GDT/IDT
- IDT and by extension IRQs are only initialised when the kernel runs
    - part of the init procedure, like the timer, etc.
- basic kernel-only GDT should be started by the bootloader while the kernel should put in another GDT with user-mode procedure
    - kernel should have direct supervision of GDT
    - we can assume everything else needed for PM is enabled (like a20)
    - make sure to flush shadow registers by changing all segment registers again.
- Software Interrupts should be set up
    - like unix systems, just use like int 0x7F (not int 0x80 because don't copy unix systems lmao)
    - keep compat with x64 SYSCALL since once kernel in 32-bit is done, 64-bit will also be made.

# 32/64 bits
- 32-bit kernels are apparently a pain to write, but might as well write it to learn
    - go back in time before going to current day
- once kernel is written in 64-bits, a UEFI bootloader can also be written.
