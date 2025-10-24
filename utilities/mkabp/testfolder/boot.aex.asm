[org 0x500]

mov     ax, 0
mov     ds, ax
mov     es, ax

mov     bx, string
call    print
jmp     $

string: db "Hello World", 0

print:
    pusha
    mov     ah, 0x0e
    loop2:
    mov     al, [bx]
    cmp     al, 0x0
    je      end
    int     0x10
    inc     bx
    jmp     loop2
    end:
    popa
    ret