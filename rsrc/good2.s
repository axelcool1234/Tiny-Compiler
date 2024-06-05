.section .text
.global _start
    # movq $0xbeefcafedeadfeeb, %r8
    # movq $0xbeefcafedeadfeeb, %r9
    # movq $0xbeefcafedeadfeeb, %r10
    # movq $0xbeefcafedeadfeeb, %r11
    # movq $0xbeefcafedeadfeeb, %r12
    # movq $0xbeefcafedeadfeeb, %r13
    # movq $0xbeefcafedeadfeeb, %r14
    # movq $0xbeefcafedeadfeeb, %r15
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
    movq $0xbeefcafedeadfeeb, %r13
    movb %dl, (%rsi)        # Write it to the buffer
    movq $0xbeefcafedeadfeeb, %r13
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


# BB1
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
movq $60, %rax    # System call number for exit
movq $0, %rdi     # Exit code 0
syscall


.section .data
    strResult: .space 16, 0
    buff: .skip 11

