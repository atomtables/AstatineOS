; booting from AEX so we should be at 0x500
; our segments are clobbered so let's fix that
mov     ax, 0x50
mov     ds, ax
mov     es, ax
mov     bx, 0x8000
mov     ss, bx
xor     sp, sp
jmp     0x50:next

next:
mov     [BOOT_DRIVE], dl
mov     [PART_START_SECTOR], edi ; receive this from the start program
cld

KERNEL_OFFSET equ 0x10000 ; The same one we used when linking the kernel

jmp     switch_to_32bit ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'
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
    ; call    READ_BIOS
    ; call    READ_BIOS_KERNEL
    
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

[bits 16]
strcmp:
    push    di
    push    si
    repe    cmpsb
    pop     si
    pop     di
    je      .match
    xor     ax, ax
    ret
.match:
    mov     ax, 1
    ret

; NEVER MIX THE BLOODY 16 AND 32
; I HATE LIFE DEBUGGED TS FOR 2 HOURS
[bits 32]
strcmp32:
    push    edi
    push    esi
    repe    cmpsb
    pop     esi
    pop     edi
    je      .match
    xor     ax, ax
    ret
.match:
    mov     ax, 1
    ret

[bits 32]
init_32bit:                 ; we are now using 32-bit instructions
                            ; 5. update the segment registers
    mov     word ax, DATA_SEG
    mov     ds, ax
    mov     ss, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    mov     ebp, 0x10000    ; 6. update the stack right at the top of the free space
    mov     esp, ebp

    mov     eax, [0x500+PART_START_SECTOR]    ; we need to get the lba
    mov     cl,  0x1    ; this command only reliably reads one sector
    mov     edi, 0x8000 ; basically a temp buffer to store
    call    ata_lba_read; TODO: ssumption that we're booting off of 0x80

    mov     si, [0x8010] ; amount of entries
    mov     eax, [0x500+PART_START_SECTOR]    ; we need to get the lba
    inc     eax; load next partition
    mov     cl,  0x1    ; this command only reliably reads one sector
    add     edi, 512
    call    ata_lba_read; TODO: ssumption that we're booting off of 0x80
    found:
    ; the first file entry is at 0x200
    mov     edi, 0x8201 ; 0x8200
    mov     word [0x7bfe], 0x8400 ; the next sector (next place we need to load at)
    mov     dword [0x7bfa], 1    ; the difference in sector
    check_name:
    cmp     si, 0   ; if the amount of files left is 0
    je      $       ; hang (there is no bootable file)
    ; check the file name (match1 is the name)
    push    esi
    mov     esi, match1+0x500
    ; DI should be at the aligned file name
    mov     cx, 7
    call    strcmp32  ; compare the file name
    ; jmp     $
    cmp     ax, 1
    pushf
    add     di, 8   ; 0x209
    popf
    pop     esi
    jne     tryagain; if it fails then just restart then and there
    push    esi
    ; now let's compare file extension
    mov     esi, match2+0x500
    mov     cx, 3
    call    strcmp32  ; compare the file extension
    cmp     eax, 1
    pop     esi
    jne     tryagain

    ; now that we've established a file, we subtract di and
    ; runexe
    add     edi, 3
    xor     edx, edx
    mov     eax, [edi + 4]  ; get the size of the file
    mov     esi, 512
    div     esi             ; divide by 512 to get amount of sectors it spans
    inc     eax             ; account for the last partial sector
    mov     ebx, eax        ; ebx now contains number of sectors (size)
    
    xor     edx, edx
    mov     eax, [edi]
    div     esi             ; divide by 512 to get amount of sectors it spans
    ; inc     eax             ; part_start_sector includes the vba, so ignore that
    add     eax, [0x500+PART_START_SECTOR]
                            ; we get the full dword of the partition area and add it
    ; eax contains LBA of the file start
    ; and edx contains the offset (% 512)
    xor     esi, esi
    mov     edi, 0x8000
    read:
        mov     cl, 1
        call    ata_lba_read
        mov     [0xb8000], 'D'
        cmp     ebx, esi
        jne     .continue
        jmp     .done
        .continue:
        inc     esi
        inc     eax
        add     edi, 512
        mov     [0xb8000], ' '
        jmp     read
        .done:
        mov     [0xb8002], 'D'
        call    CODE_SEG:0x8000
        jmp     $
    tryagain:
    ; try again
    add     di, 12  ; 20 bytes later
    sub     si, 1   ; we went through one file
    mov     dx, [0x7bfe]
    cmp     di, dx  ; make sure we aren't at the end of the sector
    jge     getnewvalues ; if we are then we load new files
    jmp     check_name
    getnewvalues:
    xor     ah, ah
    mov     eax, [0x7bfa]; get the last increment in sectors
    add     al, 1       ; add one more to ah
    mov     dword [0x7bfa], eax; and put the increment back

    add     eax, [0x500+PART_START_SECTOR]; offset by the start
    mov     cl, 0x01    ; one sector as usual
    mov     edi, 0x8200 ; same sector
    call    ata_lba_read
    ; after this point, we only read one sector.
    ; hold onto the value of bx so we can read again later
    jmp     check_name

%include "src/bootloader_partition/boot.aex/a20.asm"
%include "src/bootloader_partition/boot.aex/gdt.asm"
%include "src/bootloader_partition/boot.aex/read.asm"

BOOT_DRIVE: db 0 
PART_START_SECTOR: dd 0
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
match1: db "LOADER", 0
match2: db "AEX", 0