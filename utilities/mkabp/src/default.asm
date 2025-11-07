; this code definitely does not work i have not tested it yet
[bits   16]
; execution context: we are now still 16-bit but at 0x07c0:0x0000
; we have 512 bytes but must include signatures relating to an AB partition
jmp near   after ; two bytes
nop              ; fat32 does this (0x90)

PART_TYPE: db 0xD9  ; partition byte number (off 0x03)
PART_NAME: db "AstBoot", 0
                    ; oem identitier (hint of partition type); (off 0x4 to 0xB)
PART_SIZE: times 4 db 0xff ; total sectors in partition (max of 4GB) (off 0xC to 0xF)
LIST_SIZE: times 2 db 0xff ; amount of entries in the file list (max 65k files) (0x10/11)
VERSION:   db 0x00, 0x01   ; version number (major minor, so 0.1) (0x12/13)

after:
mov     ah, 0x0e
mov     al, 'B'
int     0x10    ; code breaks without

mov     ax, 0x07c0
mov     ds, ax
mov     es, ax
jmp     0x07c0:next
next:
; read the MBR again to get the partition table
mov     [BOOT_DRIVE], dl
call    call_lba
; check for the first partition with 0xD9
mov     si, [LIST_SIZE] ; we want to know how many files are there so when we run out we just quit.
check2:
mov     bx, 0x5c2 ; location of partition type
mov     al, [bx]
cmp     al, 0xD9
je      found
add     bx, 0x10
cmp     bx, 0x5f3
jl      check2
jmp     $
found:
mov     ah, 0x0e
mov     al, 'F'
int     0x10
; get the starting LBA of the partition
add     bx, 4       ; the lba is at 4 bytes
mov     cx, [bx]    ; get low word of starting LBA
mov     dx, [bx+2]  ; get high word of starting LBA
add     cx, 1       ; the file list is at the next sector
adc     dx, 0       ; carry flag
mov     word [lbalow4], cx
mov     word [lbalow4+2], dx ; this just sets the values of the lba (TODO: which is 32-bit? may cause errors)
mov     word [PART_START_SECTOR], cx
mov     word [PART_START_SECTOR+2], dx ; save for later
mov     word [lbaoffs], 0x7e00 ; and set the segment to 0x7e00.
; after this point, we only read one sector.
; hold onto the value of bx so we can read again later
call    call_lba
; now the file list should be at 0x7e00
; check first entry's name to see if it matches with "BOOT"
; and file extension "AEX" (astatine executable)

; the first file entry is at 0x200
mov     di, 0x201 ; 0x7e00
mov     word [0x7bfe], 0x400
mov     byte [0x7bfd], 1
check_name:
cmp     si, 0   ; if the amount of files left is 0
je      $       ; hang (there is no bootable file)
push    si
; check the file name (match1 is the name)
mov     si, match1
; DI should be at the aligned file name
mov     cx, 5
call    strcmp  ; compare the file name
cmp     ax, 1
pushf
add     di, 8   ; 0x209
popf
pop     si
jne     tryagain; if it fails then just restart then and there
push    si
; now let's compare file extension
mov     si, match2
mov     cx, 3
call    strcmp  ; compare the file extension
cmp     ax, 1
; restore si value to keep ticker indicator
pop     si
jne     tryagain

; now that we've established a file, we subtract di and
; runexe
sub     di, 9
call    runexe

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
mov     al, [0x7bfd]; get the last increment in sectors
add     al, 1       ; add one more to ah
mov     cx, [bx]    ; get low word of starting LBA
mov     dx, [bx+2]  ; get high word of starting LBA
add     cx, ax      ; the file list is at the next sector
adc     dx, 0       ; carry flag
mov     word [lbalow4], cx
mov     word [lbalow4+2], dx ; this just sets the values of the lba (TODO: which is 32-bit? may cause errors)
mov     word [lbaoffs], 0x7e00 ; and set the segment to 0x7e00.
mov     byte [0x7bfd], ah
; after this point, we only read one sector.
; hold onto the value of bx so we can read again later
call    call_lba
jmp     check_name


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

match1: db "BOOT", 0
match2: db "AEX", 0

call_lba:
    push    dx
    push    ax
    push    si
    mov     dl, [BOOT_DRIVE] ; boot drive passed by BIOS
    mov     ah, 0x42
    mov     si, lba ; set the address of the LBA packet
    int     0x13
    cmp     ah, 0x00
    jne     .error
    jc      .error
    pop     si
    pop     ax
    pop     dx
    ret
    .error:
        push    ax
        mov     ah, 0x0e
        mov     al, 'E'
        int     0x10
        pop     ax
        jmp     $

runexe:
    mov     ah, 0x0e
    mov     al, 'F'
    int     0x10
    ; given di - the offset of the entry
    ; load the executable into memory and call it
    add     di, 12  ; the offset is 13 bytes in
    mov     word ax, [di+4] ; we can also get the size of the file.
    mov     word dx, [di+6] ; high portion
    mov     si, 512
    div     si              ; divide by 512 to get amount of sectors it spans
    inc     ax              ; account for the last partial sector
    mov     cx, ax          ; save value for later

    mov     word ax, [di]   ; get location of the file
    mov     word dx, [di+2] ; high portion
    mov     si, 512
    div     si              ; divide by 512 to get amount of sectors it spans
    ;inc     eax             ; we don't account for the partial sector dumbass
    mov     esi, [PART_START_SECTOR]
    add     eax, esi          ; we get the full dword of the partition area and add it
    dec     eax               ; offsets start from the vbr so we can dec 1
    mov     ebx, eax          ; save value for next step

    ; bx has sectors to the file, cx has sector size.
    ; dx has the offset from the sector
    mov     dword [lbalow4], ebx
    mov     word [lbamaxs], cx
    mov     word [lbaoffs], 0x00
    mov     word [lbasegs], 0x50

    call    call_lba

    ; now we should be good so we call the executable at 0x0500
    ; using the remainder from before
    mov     si, 0x0500
    add     si, dx
    mov     dl, [BOOT_DRIVE]
    mov     edi, [PART_START_SECTOR] ; send this to the start program
    dec     edi ; part_start_sector should point at the vbr not the list
    jmp     0x0000:0x7c00+val ; set segment to zero so we can jump direct
    val:    jmp si  ; up up and away!

align 4
lba:
lbasize: db 0x10
lbaresv: db 0x00
lbamaxs: dw 0x0001
lbaoffs: dw 0x8000
lbasegs: dw 0x0000
lbalow4: dd 0x00000000
lbahigh: dd 0x00000000
BOOT_DRIVE: db 0
PART_START_SECTOR: dd 0

times 510 - ($ - $$) db 0 ; fill the rest of the boot sector with zeros
dw 0xAA55   ; boot sector signature