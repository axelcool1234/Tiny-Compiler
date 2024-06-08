.section .text
.global _start
write:
    movabsq $10, %rcx       # divisor
    xorq %rbx, %rbx         # count digits

_divide:
    xorq %rdx, %rdx         # High part = 0
    divq %rcx               # RAX = RDX:RAX / RCX, RDX = remainder
    pushq %rdx              # DL is a digit in range [0..9]
    incq %rbx               # Count digits
    testq %rax, %rax        # RAX is 0?
    jnz _divide             # No, continue

    # POP digits from stack in reverse order
    movq %rbx, %rcx         # Number of digits
    leaq strResult, %rsi    # RSI points to string buffer

_next_digit:
    popq %rax
    addb $'0', %al          # Convert to ASCII
    movb %al, (%rsi)        # Write it to the buffer
    incq %rsi
    decq %rcx
    jnz _next_digit          # Repeat until all digits are processed

    # Null-terminate the string
    movb $0, (%rsi)         # Null terminator

    # Prepare for sys_write
    movq $1, %rax           # sys_write system call number
    movq $1, %rdi           # File descriptor (stdout)
    leaq strResult, %rsi    # Buffer (string to print)
    movq %rbx, %rdx         # Length
    syscall                 # Invoke system call

    # Return
    ret

# Entry point for read routine
read:
    movq $0, %rax                # syscall: read
    movq $0, %rdi                # fd: stdin
    leaq buff(%rip), %rsi        # buffer to store input
    movq $11, %rdx               # max number of bytes to read
    syscall                      # make syscall

    leaq buff(%rip), %rsi        # RSI points to the input buffer
    xorq %rax, %rax              # Clear RAX (result)
    xorq %rdi, %rdi              # Clear RDI (multiplier)

_read_loop:
    movzxb (%rsi), %rcx          # Load current byte into RCX
    cmpb $0x0A, %cl              # Check for newline character
    je _read_done                # If newline, we're done
    subq $'0', %rcx              # Convert ASCII to integer
    imulq $10, %rax              # Multiply current result by 10
    addq %rcx, %rax              # Add current digit to result
    incq %rsi                    # Move to next character
    jmp _read_loop               # Repeat for next character

_read_done:
    ret

_start:

pushq %rbp
mov %rsp, %rbp
add $0, %rsp
# BB13
push $110
push $121

call function20
add $0, %rsp
popq %r15
popq %r14
popq %r13
popq %r12
popq %r10
popq %r9
popq %r8
popq %rdi
popq %rsi
popq %rdx
popq %rcx
popq %rbx
mov %rax, %rax
add $8, %rsp 
popq %rbp
movq %rbp, %rsp
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
movq $60, %rax          # sys_exit system call number
xorq %rdi, %rdi         # Status: 0
syscall                 # Invoke system call

function1:
pushq %rax
pushq %rbx
pushq %rcx
pushq %rdx
pushq %rsi
pushq %rdi
pushq %r8
pushq %r9
pushq %r10
pushq %r12
pushq %r13
pushq %r14
pushq %r15
mov %rsp, %r11
add $104, %rsp

# BB1
pop %rbx
pop %rax
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
mov $0, %r11
cmp %r11, %rbx
jne branch6

# BB2
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
ret

# BB3
branch6:

# BB4
branch10:
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
mov $0, %r11
cmp %r11, %rax
jge branch12

# BB5
branch9:
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
add %rbx, %rax
jmp branch10

# BB6
branch12:

# BB7
branch16:
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
cmp %rbx, %rax
jl branch18

# BB8
branch15:
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
sub %rbx, %rax
jmp branch16

# BB9
branch18:
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
ret
function20:
pushq %rax
pushq %rbx
pushq %rcx
pushq %rdx
pushq %rsi
pushq %rdi
pushq %r8
pushq %r9
pushq %r10
pushq %r12
pushq %r13
pushq %r14
pushq %r15
mov %rsp, %r11
add $104, %rsp

# BB10
pop %rcx
pop %rax
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
mov $0, %r11
cmp %r11, %rax
jne branch25

# BB11
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
ret

# BB12
branch25:
mov %r11, %rsp
popq %r11
pushq %rbp
mov %rsp, %rbp
add $0, %rsp
pushq %r11
push %rcx
push %rax
push %rcx

call function1
add $0, %rsp
popq %r15
popq %r14
popq %r13
popq %r12
popq %r10
popq %r9
popq %r8
popq %rdi
popq %rsi
popq %rdx
popq %rcx
popq %rbx
mov %rax, %rax
add $8, %rsp 
popq %rbp
movq %rbp, %rsp
push %rax

call function20
add $0, %rsp
popq %r15
popq %r14
popq %r13
popq %r12
popq %r10
popq %r9
popq %r8
popq %rdi
popq %rsi
popq %rdx
popq %rcx
popq %rbx
mov %rax, %rax
add $8, %rsp 
popq %rbp
movq %rbp, %rsp
ret
.section .data
    strResult: .space 16, 0
    buff: .skip 11
    newline: .byte 10
    .equ newline_len, 1
