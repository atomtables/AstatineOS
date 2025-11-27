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

; use the INT 0x15, eax= 0xE820 BIOS function to get a memory map
; note: initially di is 0, be sure to set it to a value so that the BIOS code will not be overwritten. 
;       The consequence of overwriting the BIOS code will lead to problems like getting stuck in `int 0x15`
; inputs: es:di -> destination buffer for 24 byte entries
; outputs: bp = entry count, trashes all registers except esi
[bits 16]
mmap_ent equ 0x2000             ; the number of entries will be stored at 0x2000
do_e820:
    xor di, di
    mov es, di
    mov di, 0x2004          ; Set di to 0x8004. Otherwise this code will get stuck in `int 0x15` after some entries are fetched 
	xor ebx, ebx		; ebx must be 0 to start
	xor bp, bp		; keep an entry count in bp
	mov edx, 0x534D4150	; Place "SMAP" into edx
	mov eax, 0x0000e820
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes
	int 0x15
	jc short .failed	; carry set on first call means "unsupported function"
	mov edx, 0x534D4150	; Some BIOSes apparently trash this register?
	cmp eax, edx		; on success, eax must have been reset to "SMAP"
	jne short .failed
	test ebx, ebx		; ebx = 0 implies list is only 1 entry long (worthless)
	je short .failed
	jmp short .jmpin
.e820lp:
	mov eax, 0x0000e820		; eax, ecx get trashed on every int 0x15 call
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes again
	int 0x15
	jc short .e820f		; carry set means "end of list already reached"
	mov edx, 0x0534D4150	; repair potentially trashed register
.jmpin:
	jcxz .skipent		; skip any 0 length entries
	cmp cl, 20		; got a 24 byte ACPI 3.X response?
	jbe short .notext
	test byte [es:di + 20], 1	; if so: is the "ignore this data" bit clear?
	je short .skipent
.notext:
	mov ecx, [es:di + 8]	; get lower uint32_t of memory region length
	or ecx, [es:di + 12]	; "or" it with upper uint32_t to test for zero
	jz .skipent		; if length uint64_t is 0, skip entry
	inc bp			; got a good entry: ++count, move to next storage spot
	add di, 24
.skipent:
	test ebx, ebx		; if ebx resets to 0, list is complete
	jne short .e820lp
.e820f:
	mov [es:mmap_ent], bp	; store the entry count
	clc			; there is "jc" on end of list to this point, so the carry must be cleared
	ret
.failed:
    mov [es:mmap_ent], bp	; store the entry count
	stc			; "function unsupported" error exit
	ret
[bits 16]
switch_to_32bit:
    ; before all that, get memory
    ; we want to get the memory map to store
    ; for the later kernel use.
    call   do_e820
    mov di, 0x50
    mov es, di
    ; jmp     $
    ; jc     disk_error
    ; jc     disk_error ; if carry is set, error occurred
    finished:
    ; if our drive isnt 0x80, then take precautionary measures
    mov     dl, [BOOT_DRIVE]
    ; cmp     dl, 0x80
    ; je      .continue
    ; otherwise, load via bios
    ; call    READ_BIOS
    ; call    READ_BIOS_KERNEL
    
    .continue:
    call    set_graphics_mode
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
    je      .match
    pop     esi
    pop     edi
    xor     ax, ax
    ret
.match:
    pop     esi
    pop     edi
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
    ; jmp     $
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
    mov     ecx, 7
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
    mov     ecx, 3 ; stupid ahh code i hate programming
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

%include "src/bootloader_partition/boot.aex/graphicsmode.asm"
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