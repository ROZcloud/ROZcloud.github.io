section .multiboot
align 4
    dd 0x1BADB002            ; Magic number
    dd 0x03                  ; Flags
    dd -(0x1BADB002 + 0x03)  ; Checksum

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    push ebx
    push eax
    call kernel_main
    cli
    mov edi, 0xB8000
    mov word [edi],      0x0C48 ; 'H'
    mov word [edi + 2],  0x0C41 ; 'A'
    mov word [edi + 4],  0x0C4C ; 'L'
    mov word [edi + 6],  0x0C54 ; 'T'
.hang:
    hlt
    jmp .hang

section .data


section .bss
align 16
stack_bottom:
resb 16384
stack_top:
section .note.GNU-stack noalloc noexec nowrite progbits
