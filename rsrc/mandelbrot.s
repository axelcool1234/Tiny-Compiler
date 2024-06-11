.section .text
.global _start
write:
    movabs $10, %rcx        # divisor
    xor %rbx, %rbx          # count digits
    mov %rax, %rdi          # Save original value
    cmp $0, %rax            # Compare %rax to zero
    jge _divide             # If greater than or equal to zero, skip to division

    # Handle negative number
    neg %rax                # Get the absolute value
    movb $'-', strResult    # Place the negative sign at the start of the buffer
    inc %rbx                # Increment digit count for the negative sign

_divide:
    xor %rdx, %rdx          # High part = 0
    div %rcx                # RAX = RDX:RAX / RCX, RDX = remainder
    push %rdx               # DL is a digit in range [0..9]
    inc %rbx                # Count digits
    test %rax, %rax         # RAX is 0?
    jnz _divide             # No, continue

    # POP digits from stack in reverse order
    mov %rbx, %rcx          # Number of digits
    lea strResult, %rsi     # RSI points to string buffer

    # If the number was negative, adjust the pointer for the sign
    cmp $0, %rdi              # Compare %rdi to zero
    jge _next_digit           # If greater than or equal to zero, skip to next digit loop
    inc %rsi                  # Adjust RSI to skip the negative sign
    dec %rcx                  # Decrement digit count for negative sign

_next_digit:
    pop %rax
    addb $'0', %al          # Convert to ASCII
    movb %al, (%rsi)        # Write it to the buffer
    inc %rsi
    dec %rcx
    jnz _next_digit          # Repeat until all digits are processed

    # Null-terminate the string
    movb $0, (%rsi)         # Null terminator

    # Prepare for sys_write
    mov $1, %rax            # sys_write system call number
    mov $1, %rdi            # File descriptor (stdout)
    lea strResult, %rsi     # Buffer (string to print)
    mov %rbx, %rdx          # Length
    syscall                 # Invoke system call

    # Return
    ret

# Entry point for read routine
read:
    mov $0, %rax                 # syscall: read
    mov $0, %rdi                 # fd: stdin
    lea buff(%rip), %rsi         # buffer to store input
    mov $21, %rdx                # max number of bytes to read
    syscall                      # make syscall

    lea buff(%rip), %rsi         # RSI points to the input buffer
    xor %rax, %rax               # Clear RAX (result)
    xor %rdi, %rdi               # Clear RDI (multiplier)

_read_loop:
    movzxb (%rsi), %rcx          # Load current byte into RCX
    cmpb $0x0A, %cl              # Check for newline character
    je _read_done                # If newline, we're done
    sub $'0', %rcx               # Convert ASCII to integer
    imulq $10, %rax              # Multiply current result by 10
    add %rcx, %rax               # Add current digit to result
    inc %rsi                     # Move to next character
    jmp _read_loop               # Repeat for next character

_read_done:
    ret

_start:

pushq %rbp
mov %rsp, %rbp
add $0, %rsp
# BB14
mov $0, %rax
mov $0, %rbx
mov $0, %rcx

# BB15
branch54:
mov $200, %r11
cmp %r11, %rcx
jge branch89

# BB16
mov $0, %rbx

# BB17
branch61:
mov $200, %r11
cmp %r11, %rbx
jge branch86

# BB18
mov %rbx, %rax
sub $100, %rax
push %rdx 
push %rax
push %rax
push $4
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rax
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rax
push %rdx 
push %rax
push %rax
push $10000
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rax
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rax
push %rdx 
push %rax
push %rax
push $200
mov $0, %rdx
mov 8(%rsp), %rax
cqto
idivq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rax
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rax
# SETPAR
push %rax
mov %rcx, %rax
sub $100, %rax
push %rdx 
push %rax
push %rax
push $4
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rax
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rax
push %rdx 
push %rax
push %rax
push $10000
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rax
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rax
push %rdx 
push %rax
push %rax
push $200
mov $0, %rdx
mov 8(%rsp), %rax
cqto
idivq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rax
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rax
# SETPAR
push %rax
call function1
add $-120, %rsp
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
pop %r11
mov %r11, %rsp
mov $100, %r11
cmp %r11, %rax
jne branch82

# BB19
push %rbx
push %rcx
push %rdx
push %rdi
push %rsi
push %rax
mov $8, %rax
call write
pop %rax
pop %rsi
pop %rdi
pop %rdx
pop %rcx
pop %rbx
jmp branch83

# BB20
branch82:
push %rbx
push %rcx
push %rdx
push %rdi
push %rsi
push %rax
mov $1, %rax
call write
pop %rax
pop %rsi
pop %rdi
pop %rdx
pop %rcx
pop %rbx

# BB21
branch83:
add $1, %rbx
jmp branch61

# BB22
branch86:
add $1, %rcx
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
jmp branch54

# BB23
branch89:
mov $60, %rax           # sys_exit system call number
xor %rdi, %rdi          # Status: 0
syscall                 # Invoke system call

function1:
push %rbp
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
add $120, %rsp

# BB1
pop %rax
pop %rbx
mov %rsp, %rbp
mov %r11, %rsp
mov %rbp, %r11
mov 104(%rsp), %rbp
mov %r11, 104(%rsp)
push %rbp
mov %rsp, %rbp
add $0, %rsp 
mov %rbx, %rcx
mov %rax, %rdx
mov $0, %rsi
mov $0, %rdi
mov $1, %r8

# BB2
branch4:
mov $0, %r11
cmp %r11, %r8
je branch52

# BB3
push %rdx 
push %rax
push %rcx
push %rcx
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
pop %rcx
pop %rcx
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %r9
push %rdx 
push %rax
push %rdx
push %rdx
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
pop %rdx
pop %rdx
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %r10
mov %r9, %r12
add %r10, %r12
mov $400000000, %r11
cmp %r11, %r12
jle branch23

# BB4
mov $0, %r8
jmp branch24

# BB5
branch23:

# BB6
branch24:
mov $100, %r11
cmp %r11, %rsi
jl branch29

# BB7
mov $0, %r8
jmp branch30

# BB8
branch29:

# BB9
branch30:
mov $0, %r11
cmp %r11, %r8
je branch45

# BB10
mov %r9, %rdi
sub %r10, %rdi
push %rdx 
push %rax
push %rdi
push $10000
mov $0, %rdx
mov 8(%rsp), %rax
cqto
idivq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rdi
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rdi
add %rbx, %rdi
push %rdx 
push %rax
push $2
push %rcx
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
pop %rcx
add $8, %rsp
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rcx
push %rdx 
push %rax
push %rcx
push %rdx
mov $0, %rdx
mov 8(%rsp), %rax
cqto
imulq (%rsp)
push %rax
add $8, %rsp
pop %rdx
pop %rcx
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rcx
push %rdx 
push %rax
push %rcx
push $10000
mov $0, %rdx
mov 8(%rsp), %rax
cqto
idivq (%rsp)
push %rax
add $8, %rsp
add $8, %rsp
pop %rcx
pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, %rcx
mov %rcx, %rdx
add %rax, %rdx
add $1, %rsi
mov %rdi, %rcx
jmp branch46

# BB11
branch45:

# BB12
branch46:
jmp branch4

# BB13
branch52:
add $0, %rsp
pop %rbp
add $112, %rsp
mov %rsi, %rax
ret
.section .data
    strResult: .space 21, 0
    buff: .skip 21
    newline: .byte 10
    .equ newline_len, 1
