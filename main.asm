section .data
    str_0 db "Helo2", 0
    str_1 db "Hello World", 0

section .text
global _start

_exit:
    mov eax, 1      ; exit system call
    xor ebx, ebx    ; exit code 0
    int 0x80        ; call kernel

_start:
    jmp main     ; Call the main function

main:
    jmp helloworld
__backto_main_0:
    mov eax, 1
    mov ebx, 0
    int 0x80
    jmp _exit

hello_2:
    mov eax, 4
    mov ebx, 1
    mov ecx, str_0
    mov edx, 5
    int 0x80
    jmp __backto_helloworld_1

helloworld:
    mov eax, 4
    mov ebx, 1
    mov ecx, str_1
    mov edx, 11
    int 0x80
    jmp hello_2
__backto_helloworld_1:
    jmp __backto_main_0

