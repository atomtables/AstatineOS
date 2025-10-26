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
mov     [BOOT_DRIVE], dl
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

    ; if our drive isnt 0x80, then take precautionary measures
    mov     dl, [BOOT_DRIVE]
    ; cmp     dl, 0x80
    ; je      .continue
    ; otherwise, load via bios
    call    READ_BIOS
    call    READ_BIOS_KERNEL
    
    .continue:
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
    mov     dl, [0x500+BOOT_DRIVE]
    cmp     dl, 0x80
    jmp     READ_KERNEL.done ; if we aren't reading manually then just assume above.

    mov     eax, 0x0    ; we need to get the lba
    mov     cl,  0x1    ; this command only reliably reads one sector
    mov     edi, 0x7c00 ; basically a temp buffer to store
    call    ata_lba_read; TODO: ssumption that we're booting off of 0x80
    
    mov     ebx, 0x7dc2
    .loop:
        mov     byte dl, [ebx]
        cmp     dl, 0x7F
        jne     .not_found
        ; Found the end of the string
        mov     byte [ebx], 0
        jmp     .done
        .not_found:
            add     ebx, 0x10
            cmp     ebx, 0x7df3
            jl      .loop
            jge     .failed
        .failed:
            mov     [0xb8000], 'F'
            mov     [0xb8002], 'A'
            mov     [0xb8004], 'I'
            mov     [0xb8006], 'L'
            mov     [0xb8008], 'E'
            mov     [0xb800a], 'D'
            jmp     $
        .done:
            mov     [0xb8000], 'D'
            mov     [0xb8002], 'O'
            mov     [0xb8004], 'N'
            mov     [0xb8006], 'E'
            mov     eax, [ebx+4] ; starting LBA
            mov     ebx, [ebx+8] ; ending LBA
            jmp     READ_KERNEL
READ_KERNEL:
    ; lba in memory from line above
    mov     cl,  0x1    ; read one sector at a time
    mov     edi, 0x10000 ; store at our offset
    clc
    .loop:
    call    ata_lba_read ; TODO: assumption that we're booting off of 0x80
    add     edi, 512
    inc     eax
    cmp     eax, ebx
    jl      .loop
    jge     .done
    .disk_error:
    mov     [0xb8000], 'D'
    mov     [0xb8002], 'I'
    mov     [0xb8004], 'S'
    mov     [0xb8006], 'K'
    mov     [0xb8008], ' '
    mov     [0xb800a], 'E'
    mov     [0xb800c], 'R'
    mov     [0xb800e], 'R'
    mov     [0xb8010], 'O'
    mov     [0xb8012], 'R'
    jmp     $
    .done:
    call    far CODE_SEG:0x10000
    jmp     $

[bits 16]
READ_BIOS:
    pushf
    push    dx
    push    si
    mov     dl, [BOOT_DRIVE]
    cmp     dl, 0x80
    ; je      .unnecessary

    mov     ah, 0x42
    mov     si, lbaFIRSTPART
    mov     dl, [BOOT_DRIVE]
    int     0x13
    
    mov     bx, 0x7dc2-0x500
    .loop:
        mov     byte dh, [bx]
        cmp     dh, 0x7F
        jne     .not_found
        ; Found the end of the string
        mov     byte [bx], 0
        jmp     .done
        .not_found:
            add     bx, 0x10
            cmp     bx, 0x7df3-0x500
            jl      .loop
            jge     .failed
        .failed:
            mov     ah, 0x0e
            mov     al, 'F'
            int     0x10
            mov     dl, [BOOT_DRIVE]
            jmp     $
        .done:
            mov     eax, [ebx+4] ; starting LBA
            mov     ebx, [ebx+8] ; size LBA
            add     ebx, eax    ; make that ending rq
    .unnecessary:
        pop     si
        pop     dx
        popf
        ret

; proof we can use r32 in real mode
[bits 16]
READ_BIOS_KERNEL:
    pushf
    pusha
    mov     ecx, ebx
    mov     [lbalow4], eax
    .loop:
    xor     eax, eax
    mov     ah, 0x42
    mov     si, lba
    mov     dl, [BOOT_DRIVE]
    int     0x13
    jc      .failed
    ; update the location
    mov     dx, [lbasegs]
    add     dx, 32
    mov     [lbasegs], dx
    ; update LBA
    mov     dword eax, [lbalow4]
    inc     eax
    mov     dword [lbalow4], eax
    cmp     eax, ecx
    ; if less, then we gucci
    jl      .loop
    jge     .done
    .failed:
    push    ax
    mov     ah, 0x0e
    mov     al, 'F'
    int     0x10
    pop     ax
    jmp     $
    .done:
    popa
    popf
    ret

%include "src/stage2/a20.asm"
%include "src/stage2/gdt.asm"
%include "src/stage2/read.asm"

BOOT_DRIVE: db 0
align 4
lba:
lbasize: db 0x10
lbaresv: db 0x00
lbamaxs: dw 0x0001
lbaoffs: dw 0x0000
lbasegs: dw 0x1000
lbalow4: dd 0x00000000
lbahigh: dd 0x00000000
align 4
lbaFIRSTPART:
db 0x10
db 0x00
dw 0x0001
dw 0x7c00
dw 0x0000
dd 0x00000000
dd 0x00000000