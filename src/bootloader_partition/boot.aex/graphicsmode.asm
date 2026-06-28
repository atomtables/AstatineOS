[bits 16]

; si: null terminated string
; return cx: length
[bits 16]
strlen:
    push    ax
    push    si
    xor     cx, cx
.loop:
    lodsb
    cmp     al, 0
    je      .end
    inc     cx
    jmp     .loop
.end:
    pop     si
    pop     ax
    ret

; si: null terminated string
; bl: color attribute
; dh: row
; dl: column
[bits 16]
draw_string:
    push    ax
    push    bx
    push    cx
    push    si
    push    bp
    ; we can just use bios write string
    ; but have to get length of string first
    call    strlen
    xor     bh, bh
    mov     ah, 0x13
    mov     al, 0x01
    mov     bp, si
    int     0x10
    .end:
    pop     bp
    pop     si
    pop     cx
    pop     bx
    pop     ax
    ret

[bits 16]
choose_graphics_mode:
    pusha
    ; clear the screen
    mov     ah, 0x06
    mov     al, 0x00
    mov     bh, 0x07
    mov     ch, 0x00
    mov     cl, 0x00
    mov     dh, 0x18
    mov     dl, 0x4F
    int     0x10
    ; print the first line
    mov     dh, 8
    mov     dl, 16
    mov     bl, 0x0f
    mov     si, os_identifier
    call    draw_string
    ; now we print the second line
    mov     dh, 9
    mov     dl, 16
    mov     bl, 0x07
    mov     si, os_callforaction
    call    draw_string
    ; now we can start drawing options
    .loop:
    ; draw the first option
    mov     dh, 11
    mov     dl, 16
    ; but now we check if its selected (0x70) or not (0x07)
    movzx   di, byte [currently_selected_graphics_mode]
    cmp     di, 0
    jne     .second_selected
    .first_selected:
    mov     bl, 0x70
    mov     si, os_textmode
    call    draw_string
    ; second string
    mov     dh, 12
    mov     dl, 16
    mov     bl, 0x07
    mov     si, os_bitmapmode
    call    draw_string
    jmp     .keypress
    .second_selected:
    mov     bl, 0x07
    mov     si, os_textmode
    call    draw_string
    ; second string
    mov     dh, 12
    mov     dl, 16
    mov     bl, 0x70
    mov     si, os_bitmapmode
    call    draw_string
    .keypress:
    mov     ah, 0x00
    int     0x16
    ; in ah is the scan code
    cmp     ah, 0x1C    ; enter
    je      .done
    cmp     ah, 0x48    ; arrow down
    je      .arrow_down
    cmp     ah, 0x50    ; arrow up
    je      .arrow_up
    jmp     .keypress
    .arrow_up:
    mov     dh, byte [currently_selected_graphics_mode]
    cmp     dh, 0
    je      .allow_arrow_up
    jmp     .keypress
    .allow_arrow_up:
    inc     byte [currently_selected_graphics_mode]
    jmp     .loop
    .arrow_down:
    mov     dh, byte [currently_selected_graphics_mode]
    cmp     dh, 1
    je      .allow_arrow_down
    jmp     .keypress
    .allow_arrow_down:
    dec     byte [currently_selected_graphics_mode]
    jmp     .loop
    .done:
    movzx   di, byte [currently_selected_graphics_mode]
    cmp     di, 1
    jne     .end
    .set_graphics:
    call    set_graphics_mode
    .end:
    mov     ax, 0
    mov     gs, ax
    mov     ax, di
    mov     byte [gs:0x1000], al
    popa
    ret

[bits 16]
set_graphics_mode:
    push   ax
    mov    ah, 0x00
    mov    al, 0x13
    int    0x10
    pop    ax
    ret

[bits 16]
get_vesa_info:
    clc
    mov     ax, 0x4F00          ; VBE function 00h - Return VBE Controller Information
    mov     di, buffer   ; ES:DI -> VBEInfoBlock
    int     0x10
    jne     .failed
    ret
    .failed:
        stc
        ret

buffer: resb 512

failed_vbe_info_str: db "Failed to get VBE info, continuing without graphics...", 0

; =====================================================================
; VBE Controller Information Block (512 bytes total)
; Used with INT 10h, AX = 4F00h
; =====================================================================

struc VBEInfoBlock
    .Signature:        resb 4      ; 'VESA' after call
    .Version:          resw 1      ; BCD: 0x0200 = VBE 2.0
    .OemStringPtr:     resd 1      ; Far pointer: seg:off
    .Capabilities:     resb 4
    .VideoModePtr:     resd 1      ; Far pointer to mode list
    .TotalMemory:      resw 1      ; In 64 KB blocks

    ; VBE 2.0+ additional fields
    .OemSoftwareRev:   resw 1
    .OemVendorNamePtr: resd 1
    .OemProductNamePtr resd 1
    .OemProductRevPtr: resd 1

    .Reserved:         resb 222    ; Must be zero
    .OemData:          resb 256    ; OEM data
endstruc

; =====================================================================
; VBE Mode Information Block (256 bytes total)
; Used with INT 10h, AX = 4F01h
; =====================================================================

struc VBEModeInfo
    .ModeAttributes:         resw 1
    .WinAAttributes:         resb 1
    .WinBAttributes:         resb 1
    .WinGranularity:         resw 1
    .WinSize:                resw 1
    .WinASegment:            resw 1
    .WinBSegment:            resw 1
    .WinFuncPtr:             resd 1
    .BytesPerScanLine:       resw 1

    ; VBE 1.2 fields
    .XResolution:            resw 1
    .YResolution:            resw 1
    .XCharSize:              resb 1
    .YCharSize:              resb 1
    .NumberOfPlanes:         resb 1
    .BitsPerPixel:           resb 1
    .NumberOfBanks:          resb 1
    .MemoryModel:            resb 1
    .BankSize:               resb 1
    .NumberOfImagePages:     resb 1
    .Reserved1:              resb 1

    ; Direct color fields (VBE 2.0)
    .RedMaskSize:            resb 1
    .RedFieldPosition:       resb 1
    .GreenMaskSize:          resb 1
    .GreenFieldPosition:     resb 1
    .BlueMaskSize:           resb 1
    .BlueFieldPosition:      resb 1
    .RsvdMaskSize:           resb 1
    .RsvdFieldPosition:      resb 1
    .DirectColorModeInfo:    resb 1

    ; Linear framebuffer fields
    .PhysBasePtr:            resd 1
    .OffScreenMemOffset:     resd 1
    .OffScreenMemSize:       resw 1

    .Reserved2:              resb 206
endstruc

