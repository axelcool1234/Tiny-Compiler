.section .text
.global _start
write:
    movabs $10, %rcx
    xor %rbx, %rbx

_divide:
    xor %rdx, %rdx
    div %rcx
    push %rdx
    inc %rbx
    test %rax, %rax
    jnz _divide


    mov %rbx, %rcx
    lea strResult, %rsi

_next_digit:
    pop %rax
    add $'0', %al
    mov %al, (%rsi)
    inc %rsi
    dec %rcx
    jnz _next_digit


    mov $0, (%rsi)


    mov $1, %rax
    mov $1, %rdi
    lea strResult, %rsi
    mov %rbx, %rdx
    syscall


    ret


read:
    mov $0, %rax
    mov $0, %rdi
    lea buff(%rip), %rsi
    mov $11, %rdx
    syscall

    lea buff(%rip), %rsi
    xor %rax, %rax
    xor %rdi, %rdi

_read_loop:
    movzx (%rsi), %rcx
    cmp $0x0A, %cl
    je _read_done
    sub $'0', %rcx
    imul $10, %rax
    add %rcx, %rax
    inc %rsi
    jmp _read_loop

_read_done:
    ret

_start:

push %rbp
mov %rsp, %rbp
add $0, %rsp

push $110
push $121

call function20
add $0, %rsp
pop %r15
pop %r14
pop %r13
pop %r12
pop %r10
pop %r9
pop %r8
pop %rdi
pop %rsi
pop %rdx
pop %rcx
pop %rbx
mov %rax, %rax
add $8, %rsp
pop %rbp
mov %rbp, %rsp
push %rbx
push %rcx
push %rdx
push %rdi
push %rsi
call write
pop %rsi
pop %rdi
pop %rdx
pop %rcx
pop %rbx
push %rax
push %rdi
push %rsi
push %rdx
push %rcx
mov $1, %rax
mov $1, %rdi
mov $newline, %rsi
mov $newline_len, %rdx
syscall
pop %rcx
pop %rdx
pop %rsi
pop %rdi
pop %rax
mov $60, %rax
xor %rdi, %rdi
syscall

function1:
push %rax
push %rbx
push %rcx
push %rdx
push %rsi
push %rdi
push %r8
push %r9
push %r10
push %r12
push %r13
push %r14
push %r15
mov %rsp, %r11
add $104, %rsp


pop %rbx
pop %rax
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
mov $0, %r11
cmp %r11, %rbx
jne branch6


mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
ret


branch6:


branch10:
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
mov $0, %r11
cmp %r11, %rax
jge branch12


branch9:
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
add %rbx, %rax
jmp branch10


branch12:


branch16:
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
cmp %rbx, %rax
jl branch18


branch15:
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
sub %rbx, %rax
jmp branch16


branch18:
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
ret
function20:
push %rax
push %rbx
push %rcx
push %rdx
push %rsi
push %rdi
push %r8
push %r9
push %r10
push %r12
push %r13
push %r14
push %r15
mov %rsp, %r11
add $104, %rsp


pop %rcx
pop %rax
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
mov $0, %r11
cmp %r11, %rax
jne branch25


mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
ret


branch25:
mov %r11, %rsp
pop %r11
push %rbp
mov %rsp, %rbp
add $0, %rsp
push %r11
push %rcx
push %rax
push %rcx

call function1
add $0, %rsp
pop %r15
pop %r14
pop %r13
pop %r12
pop %r10
pop %r9
pop %r8
pop %rdi
pop %rsi
pop %rdx
pop %rcx
pop %rbx
mov %rax, %rax
add $8, %rsp
pop %rbp
mov %rbp, %rsp
push %rax

call function20
add $0, %rsp
pop %r15
pop %r14
pop %r13
pop %r12
pop %r10
pop %r9
pop %r8
pop %rdi
pop %rsi
pop %rdx
pop %rcx
pop %rbx
mov %rax, %rax
add $8, %rsp
pop %rbp
mov %rbp, %rsp
ret
.section .data
    strResult: .space 16, 0
    buff: .skip 11
    newline: .byte 10
    .equ newline_len, 1
