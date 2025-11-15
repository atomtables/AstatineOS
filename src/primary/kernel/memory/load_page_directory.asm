; extern void load_page_directory(u32 pd_addr);
[bits 32]
global load_page_directory
load_page_directory:
    mov eax, [esp + 4] ; get the page directory address from the stack
    mov cr3, eax       ; load it into cr3

    ; Enable paging by setting the PG bit in CR0
    mov eax, cr0
    or eax, 0x80000000 ; set the PG bit (bit 31)
    mov cr0, eax
    
    ret