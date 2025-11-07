; [bits 32]
; global _start
; [bits 32]
; _start:
;     mov [0xb8000], ' '
;     [extern main] ; Define calling point. Must have same name as kernel.c 'main' function
;     call main ; Calls the C function. The linker will know where it is placed in memory
;     jmp $
; doesn't work