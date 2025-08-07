[bits   16]

jmp     long 0x7c0:next    ; Jump to add CS 0x7c00

align   4
lba:
    lbasize: db 0x10
    lbaresv: db 0x00
    lbamaxs: dw 0x007F
    lbaoffs: dw 0x0000
    lbasegs: dw 0x07c0
    lbalow4: dd 0x00000001
    lbahigh: dd 0x00000000

next:
; relocate boot code to 0x500
mov     ax, 0x0000
mov     ds, ax
mov     es, ax
cld
mov     si, 0x7c00
mov     di, 0x0500
mov     cx, 0x0200
rep     movsb
jmp     0x0050:resume ; resume execution at 0x0500

resume:
; set general parameters
xor     ax, ax
mov     bx, 0x8000
mov     ss, bx  ; set the stack segment to 0x8000
mov     sp, ax  ; set the pointer to 0x0, stack starts at 0x80000.
mov     ax, 0x0050
mov     ds, ax  ; set the data segment
mov     es, ax  ; and the extra segment to code segment (0x7c00)

mov     [BOOT_DRIVE], dl    ; store boot drive in a more persistent place

; find the active partition
mov     bx, 0x1be ; 0x1be, 1ce, 1de, 1ee show the active partition (main booting partition)
loop:
mov     ax, [ds:bx]
and     ax, 0x0080
cmp     ax, 0x0080
je      bootto      ; if active, jump to boot routine
add     bx, 0x0010
cmp     bx, 0x01ef
jge     packitup    ; if we've seen all 4 partitions, there is no active partition
jmp     loop

bootto:
;    mov     cx, 4       ; copy 4 bytes (size of lower LBA address)
;    mov     si, bx      ; Source offset (active paritition pointer)
;    add     si, 0x08    ; since the LBA is at an offset of 8 from the active partition byte
;    mov     di, lowerlba; Destination offset
;    rep     movsb   ; movsb moves DS:SI to ES:DI CX times
;                    ; so in effect, we are copying the LBA of the partition
;                    ; into the packet

    mov     si, lba ; set the address of the LBA packet
    mov     ah, 0x42; BIOS call to load sectors via LBA
    mov     dl, [BOOT_DRIVE] ; just in case, load DL
    int     0x13    ; read the first sector of the active partition
    jc      packitup ; if error, jump to error routine

    jmp     long 0x07c0:0x0000 ; jump back to partition

packitup:
    mov     ah, 0x0e
    mov     bx, error
    loop2:
    mov     al, [bx]
    cmp     al, 0x0
    je      end
    int     0x10
    inc     bx
    jmp     short loop2
    end:
    jmp     $

BOOT_DRIVE: db 0             ; It is a good idea to store it in memory because 'dl' may get overwritten
error: db "AstatineOS Bootloader failed to find an active/bootable partition."

times 446 - ($-$$) db 0
partition_1:
    db 0x80                    ; Drive attribute: 0x80 = active/bootable
    db 0x00, 0x01, 0x01        ; CHS address of partition start (Cylinder 0, Head 1, Sector 1)
    db 0xD9                    ; Partition type: 0xD9 (AstatineOS Bootstrap Partition)
    db 0x00, 0x05, 0x01        ; CHS address of partition end (approx. Cylinder 79, Head 1, Sector 18)
    dd 0x00000001              ; LBA of partition start (sector 1, as sector 0 is the MBR)
    dd 0x00000271              ; Number of sectors in partition (1440 sectors for 1.44 MB)
partition_2:
    db 0x00                    ; Drive attribute: 0x80 = active/bootable
    db 0x00, 0x01, 0x01        ; CHS address of partition start (Cylinder 0, Head 1, Sector 1)
    db 0xD9                    ; Partition type: 0xD9 (AstatineOS Bootstrap Partition)
    db 0x00, 0x0F, 0x12        ; CHS address of partition end (approx. Cylinder 79, Head 1, Sector 18)
    dd 0x00000272              ; LBA of partition start (sector 1, as sector 0 is the MBR)
    dd 0x000005A0              ; Number of sectors in partition (1440 sectors for 1.44 MB)
times 16 db 0                  ; Entry 3 (blank)
times 16 db 0                  ; Entry 4 (blank)

dw 0xaa55