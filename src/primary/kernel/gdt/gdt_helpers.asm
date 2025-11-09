global flush_tss
; void flush_tss()
flush_tss:
	mov ax, (5 * 8) | 0 ; fifth 8-byte selector, symbolically OR-ed with 0 to set the RPL (requested privilege level).
	ltr ax
	ret

global load_gdt
; void load_gdt(void* descriptor)
load_gdt:
	mov     eax, [esp+4]
	cli
	lgdt    [eax]
	; 0x8 is our kernel code segment
	jmp     0x8:.rewrite
	.rewrite:
	; 0x10 is our kernel data segment
	mov     word ax, 0x10
    mov     ds, ax
    mov     ss, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
	; don't re-enable interrupts (handled by IDT)
	ret