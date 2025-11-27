[bits 16]

set_graphics_mode:
    push   ax
    ; mov    ah, 0x00
    ; mov    al, 0x10          ; Set VESA mode 0x10 (640x480x16)
    ; int    0x10
    pop    ax
    ret

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

