ALIGN 8
gdt_start:  ; don't remove the labels, they're needed to compute sizes and jumps
            ; the GDT starts with a null 8-byte
    dd  0x0 ; 4 byte
    dd  0x0 ; 4 byte

; GDT for kernel code segment. base = 0x00000000, length = 0xfffff
; for flags, refer to os-dev.pdf document, page 36
gdt_code:
    dw  0xffff    ; segment length, bits 0-15 (16-bit value)
    dw  0x0       ; segment base, bits 0-15 (16-bit value)
    db  0x0       ; segment base, bits 16-23 (8-bit value)
    db  10011010b ; flags (8 bits)
    db  11001111b ; flags (4 bits) + segment length, bits 16-19
    db  0x0       ; segment base, bits 24-31

; GDT for kernel data segment. base and length identical to code segment
; some flags changed, again, refer to os-dev.pdf
gdt_data:
    dw  0xffff
    dw  0x0
    db  0x0
    db  10010010b
    db  11001111b
    db  0x0

; Base = 0
; Limit = 0xFFFFF
; Access Byte = 0xFA
; Flags = 0xC
;gdt_user_code:
;    dw  0xffff
;    dw  0x0
;    db  0x0
;    db  10011010b
;    db  11001111b
;    db  0x0

; Base = 0
; Limit = 0xFFFFF
; Access Byte = 0xF2
; Flags = 0xC
;gdt_user_data:
;    dw  0xffff
;    dw  0x0
;    db  0x0
;    db  10010010b
;    db  11001111b
;    db  0x0

; gdt taask segment
;gdt_tss:
;    dw  0x67
;    dw  0x0
;    db  0x0
;    db  10001001b
;    db  0x0
;    db  0x0

gdt_end:

; GDT descriptor
gdt_descriptor:
    dw  gdt_end - gdt_start - 1 ; size (16 bit), always one less of its true size
    dd  gdt_start+0x500 ; address (32 bit)

; define some constants for later use
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start