.section .text
.global _start
write:
    mov $0xbeefcafebeefcafe, %r8
    movabs $10, %rcx
    xor %rbx, %rbx

_divide:
    xor %rdx, %rdx
    div %rcx
    push %rdx
    inc %rbx
    cmp $0, %rax
    jne _divide


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



push %rdi
push %rsi
push %rdx
push %rcx
call read
pop %rcx
pop %rdx
pop %rsi
pop %rdi
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
mov $60, %rax
mov $0, %rdi
syscall


.section .data
    strResult: .space 16, 0
    buff: .skip 11
