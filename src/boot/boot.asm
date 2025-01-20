[bits   16]

jmp     0x7c0:next    ; Jump to add CS 0x7c00

lba:
    lbasize: db 0x10
    lbaresv: db 0x00
    lbamaxs: dw 0x007F
    lbaoffs: dw 0x0000
    lbasegs: dw 0x1000
    lbalow4: dd 0x00000001
    lbahigh: dd 0x00000000


next:
mov     [BOOT_DRIVE], dl

xor     ax, ax
mov     bx, 0x8000
mov     ss, bx
mov     sp, ax

cld

KERNEL_OFFSET equ 0x10000 ; The same one we used when linking the kernel

mov     ax, 0x7c0
mov     ds, ax
mov     es, ax

mov     si, lba
mov     ah, 0x42
int     0x13
jc      disk_error

call    switch_to_32bit ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'
jmp     $               ; Never executed

disk_error:
    ; mov     si, disk_error_msg
    ; Print the error message by moving it to the video memory
    mov     dl, ah
    mov     ah, 0x0E
    mov     al, 'E'
    int     0x10
done:
    jmp     $

[bits 16]
switch_to_32bit:
    ; before all that, get memory
    XOR CX, CX
    XOR DX, DX
    MOV AX, 0xE801
    INT 0x15		; request upper memory size
    JC disk_error
    JCXZ .USEAX		; was the CX result invalid?

    MOV AX, CX
    MOV BX, DX
    .USEAX:
    ; AX = number of contiguous Kb, 1M to 16M
    ; BX = contiguous 64Kb pages above 16M
    mov     cx, ax
    xor     ax, ax
    mov     fs, ax
    mov     word [fs:0x500], cx
    mov     word [fs:0x502], bx
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