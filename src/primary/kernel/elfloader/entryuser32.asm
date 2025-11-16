[bits 32]

global entryuser32
; void entryuser32(u32 address, u32 useresp);
entryuser32:
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax ; SS is handled by iret

	; set up the stack frame iret expects
	mov eax, ecx
	push (4 * 8) | 3 ; data selector
	push eax ; current esp
	pushf ; eflags
	push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	push ebx ; instruction address to return to
	iret