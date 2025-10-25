; booting from AEX so we should be at 0x500
; our segments are clobbered so let's fix that
mov     ax, 0x50
mov     ds, ax
mov     es, ax
mov     bx, 0x8000
mov     ss, bx
mov     sp, ax
jmp     0x50:next

next:
cld

KERNEL_OFFSET equ 0x10000 ; The same one we used when linking the kernel

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
    mov     word [fs:0x8000], cx
    mov     word [fs:0x8002], bx
    call    enable_a20          ; 0. enable A20 line
    cli                         ; 1. disable interrupts
    nop                         ; 1.1. some CPUs require a delay after cli
    lgdt    [gdt_descriptor]    ; 2. load the GDT descriptor
    mov     eax, cr0
    or      eax, 0x1            ; 3. set 32-bit mode bit in cr0
    mov     cr0, eax
                                ; 4. far jump by using a different segment
    jmp     CODE_SEG:init_32bit+0x500

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

    jmp     READ     ; 7. Call a well-known label with useful code

[bits 32]
READ:
    mov     eax, 0x0    ; we need to get the lba
    mov     cl,  0x1    ; this command only reliably reads one sector
    mov     edi, 0x7c00 ; basically a temp buffer to store
    mov     [0xb8000], 'F'
    jmp     $
    call    ata_lba_read
    jmp     $

%include "src/stage2/a20.asm"
%include "src/stage2/gdt.asm"
%include "src/stage2/read.asm"