.section .text
.global _start
write:
    movq $0xbeefcafe, %rdx
    leaq digitSpace(%rip), %rcx   # Load address of digitSpace into RCX
    movq $10, %rbx                # Load constant 10 into RBX
    movq %rbx, (%rcx)             # Store RBX at the address pointed by RCX
    leaq digitSpacePos(%rip), %rcx  # Load address of digitSpacePos into RCX
    movq %rcx, digitSpacePos(%rip)  # Update digitSpacePos with new value of RCX

_write_loop:
    xorq %rdx, %rdx               # Zero out RDX
    movq $10, %rbx                # Load constant 10 into RBX
    divq %rbx                     # Divide RAX by RBX; quotient in RAX, remainder in RDX
    pushq %rax                    # Push RAX onto stack for later
    addq $48, %rdx                # Convert remainder to ASCII

    movq digitSpacePos(%rip), %rcx  # Load current digitSpacePos into RCX
    movb %dl, (%rcx)              # Store low byte of RDX at [RCX]
    incq %rcx                     # Increment RCX
    movq %rcx, digitSpacePos(%rip) # Store updated RCX back to digitSpacePos

    popq %rax                     # Restore original RAX
    testq %rax, %rax              # Test RAX for zero
    jne _write_loop               # If not zero, loop

_write_loop2:
    movq digitSpacePos(%rip), %rcx  # Get current digitSpacePos

    movq $1, %rax                # syscall: write
    movq $1, %rdi                # fd: stdout
    movq %rcx, %rsi              # buf: current position in digitSpace
    movq $1, %rdx                # count: 1
    syscall                      # make syscall

    movq digitSpacePos(%rip), %rcx  # Load current digitSpacePos
    decq %rcx                     # Decrement RCX
    movq %rcx, digitSpacePos(%rip) # Update digitSpacePos

    cmpq %rcx, digitSpace(%rip)  # Compare with start of digitSpace
    jge _write_loop2             # If RCX >= digitSpace, repeat

    cld                          # Clear direction flag
    movq $100, %rcx              # Set count to 100 bytes
    leaq digitSpace(%rip), %rdi  # Set destination pointer to digitSpace
    movb $0, %al                 # Set value to be stored
    rep stosb                    # Store AL in memory, incrementing EDI, repeat ECX times

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
    test1: .long 3203386110
    newline: .byte 10
    .equ newline_len, 1
    digitSpace: .skip 100
    digitSpacePos: .skip 8
    buff: .skip 11
    test4: .long 65535
    test5: .long 65535

