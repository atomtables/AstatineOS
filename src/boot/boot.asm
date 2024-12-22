[bits   16]

jmp     0x7c0:$+2    ; Jump to add CS 0x7c00

xor     ax, ax
mov     bx, 0x8000
mov     ss, bx
mov     sp, ax

cld

KERNEL_OFFSET equ 0x10000 ; The same one we used when linking the kernel

mov     bx, 0x1000
mov     es, bx
mov     bx, 0x0000 ; es:bx = 0x7E00
mov     ah, 0x02 ; ah <- int 0x13 function. 0x02 = 'read'
mov     al, 0x40 ; al <- number of sectors to read (0x01 .. 0x80)
mov     cl, 0x02 ; cl <- sector (0x01 .. 0x11)
mov     ch, 0x00 ; ch <- cylinder (0x0 .. 0x3FF, upper 2 bits in 'cl')
mov     dh, 0x00 ; dh <- head number (0x0 .. 0xF)

int     0x13     ; BIOS interrupt
jc      .disk_error

mov     ax, 0x7c0
mov     ds, ax
mov     es, ax

call    switch_to_32bit ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'
jmp     $               ; Never executed

.disk_error:
    mov     si, disk_error_msg
    ; Print the error message by moving it to the video memory
    mov     ah, 0x0E
.repeat:
    mov     al, [si]
    inc     si
    cmp     al, 0
    je      done
    int     0x10
    jmp     .repeat
done:
    jmp     $

[bits 16]
switch_to_32bit:
    call    enable_a20          ; 0. enable A20 line
    cli                         ; 1. disable interrupts
    nop                         ; 1.1. some CPUs require a delay after cli
    lgdt    [gdt_descriptor]    ; 2. load the GDT descriptor
    mov     eax, cr0
    or      eax, 0x1            ; 3. set 32-bit mode bit in cr0
    mov     cr0, eax
                                ; 4. far jump by using a different segment
    jmp     CODE_SEG:init_32bit+0x7c00

[bits 32]
init_32bit:                 ; we are now using 32-bit instructions
                            ; 5. update the segment registers
    mov     word ax, DATA_SEG
    mov     ds, ax
    mov     ss, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    mov     ebp, 0x90000    ; 6. update the stack right at the top of the free space
    mov     esp, ebp

    call    BEGIN_32BIT     ; 7. Call a well-known label with useful code

[bits 32]
BEGIN_32BIT:
    call    CODE_SEG:0x10000         ; Give control to the kernel
    jmp     $               ; Stay here when the kernel returns control to us (if ever)

%include "gdt.asm"
%include "a20.asm"

BOOT_DRIVE db 0             ; It is a good idea to store it in memory because 'dl' may get overwritten

times 446 - ($-$$) db 0
partition_1:
    db 0x80                    ; Drive attribute: 0x80 = active/bootable
    db 0x00, 0x01, 0x01        ; CHS address of partition start (Cylinder 0, Head 1, Sector 1)
    db 0xFF                    ; Partition type: 0xFF (custom/vendor-specific)
    db 0x00, 0x0F, 0x12        ; CHS address of partition end (approx. Cylinder 79, Head 1, Sector 18)
    dd 0x00000001              ; LBA of partition start (sector 1, as sector 0 is the MBR)
    dd 0x000005A0              ; Number of sectors in partition (1440 sectors for 1.44 MB)
; padding
times 16 db 0                ; Entry 2
times 16 db 0                ; Entry 3
times 16 db 0                ; Entry 4

dw 0xaa55

disk_error_msg db "Disk error", 0