[bits   16]
; execution context: we are now still 16-bit but at 0x07c0:0x0000 (but still don't assume)

; step 1: set up segment registers
mov     ax, cs      ; dk where we running so set segments to CS
mov     ds, ax      ; set data segment
mov     es, ax      ; set extra segment
mov     dx, ax

mov     ax, 0x8000
mov     ss, ax      ; set stack segment to 0x8000

; step 2: disable interrupts (including NMIs);
cli                 ; disable interrupts

in      al, 0x70    ; read NMI port
or      al, 0x80    ; clear NMI bit
out     0x70, al    ; write back to NMI port
in      al, 0x71    ; OSDEV.org suggests to do this idk why

; step 3: enable A20 line
call    enable_a20  ; should use the OSDEV a20.asm code
jc      $           ; if error, halt, TODO: add error message

; step 4: load GDT (from gdt.asm)
lgdt    [gdt_descriptor] ; load the GDT

; step 5: set cr0.PE
mov     eax, cr0
or      eax, 1       ; set PE bit
mov     cr0, eax     ; write back to cr0

; step 6: "far" jump to 32-bit land to further continue
jmp     CODE_SEG:0x7c00+kernelspace;doesn't error during compilation

[bits   32]

kernelspace:
; step 7: set segment registers to match with GDT
mov     word ax, DATA_SEG;update everything to kernel data segment
mov     ds, ax
mov     ss, ax
mov     es, ax
mov     fs, ax
mov     gs, ax
mov     ebp, 0x80000    ; reset the stack pointer
mov     esp, ebp

; step 8: short jump to become a true 32 bit crusher
actualspace:
mov     byte [0xb8000], 'H'
jmp     $

%include "gdt.asm"
%include "a20.asm"

times 510 - ($ - $$) db 0 ; fill the rest of the boot sector with zeros
dw 0xAA55 ; boot sector signature