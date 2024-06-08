#include "codeemitter.hpp"
#include <format>
#include <iostream>

CodeEmitter::CodeEmitter(IntermediateRepresentation&& ir) : ir(std::move(ir)), ofile("a.out") {}

void CodeEmitter::debug() const {
    ir.debug();
}

void CodeEmitter::emit_code() {
    static const std::string data_section = 
R"(.section .data
    strResult: .space 16, 0
    buff: .skip 11
    newline: .byte 10
    .equ newline_len, 1
)";

    static const std::string default_text_section = 
R"(.section .text
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

)";
    static const std::string exit = 
R"(movq $60, %rax          # sys_exit system call number
xorq %rdi, %rdi         # Status: 0
syscall                 # Invoke system call

)"; 
   std::string program_string;

    // mov rsp rbp // TODO
    // sub {spill_count * 8}, rsp // TODO

    // Emit main blocks
    program_string += std::format("pushq %rbp\nmov %rsp, %rbp\nadd ${}, %rsp", -8 * ir.spill_count);
    for(const auto& b : ir.get_basic_blocks()) {
        if(!(b.index >= ir.get_successors(0).back())) continue;
        program_string += std::format("\n# BB{}\n", b.index);
        program_string += block(b.index);
    }

    program_string += exit;

    // add, rsp, {spill_count * 8} // emit when we run into a return //TODO

    // Emit function blocks
    for(size_t index = 0; index < ir.get_successors(0).size() - 1; ++index) {
        program_string += std::format("function{}:\n", ir.get_instructions(ir.get_successors(0).at(index)).at(0).instruction_number);
        // program_string += std::format("pushq %rbp\nmov %rsp, %rbp\nadd ${}, %rsp", -8 * ir.spill_count);
        program_string += R"(push %rbp
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
add $120, %rsp
)";
        getting_pars = true;
        for(bb_t func_index = ir.get_successors(0).at(index); func_index < ir.get_successors(0).at(index+1); ++func_index) {
            program_string += std::format("\n# BB{}\n", func_index);
            program_string += block(func_index);
        }
    }
    ofile << default_text_section << program_string << data_section; 
}

std::string CodeEmitter::block(const bb_t& b) {
    std::string block_string;

    // If block is already emitted, skip.
    if(ir.is_emitted(b)) return "";

    // Ensure all predecessor blocks have been emitted first.
    for(const auto& predecessor : ir.get_predecessors(b)) {
        if(predecessor == 0) continue;
        if(!ir.is_emitted(predecessor)) return "";
    }

    // Emit instructions in given block.
    if(ir.get_instructions(b).size() != 0) {

        // Under the following conditions, a block may need a label for other blocks to branch to it.
        const Blocktype& t = ir.get_type(b);
        if(t == Blocktype::JOIN || t == Blocktype::IF_BRANCH || t == Blocktype::WHILE_BRANCH || ir.is_branch_back(b) || ir.is_loop_header(b)) {
            block_string += std::format("branch{}:\n", ir.get_instructions(b).at(0).instruction_number);
        }

        // Emit instructions
        for(const auto& instruct : ir.get_instructions(b)) {
            block_string += instruction(instruct);
        }
    }

    // Emit the branch instruction of the given block.
    if(ir.has_branch_instruction(b)) {
        block_string += instruction(ir.get_branch_instruction(b));
    }

    // Block is now emitted.
    ir.set_emitted(b);

    return block_string;
}

std::string CodeEmitter::branch(const instruct_t& i, const std::string& opcode) {
    return std::format("{} branch{}\n", opcode, i);
}

std::string CodeEmitter::write(const Instruction& instruction) {
    // rax, rbx, rcx, rdx, rsi, rdi
    std::string result = "push %rbx\npush %rcx\npush %rdx\npush %rdi\npush %rsi\n";
    if(!ir.is_const_instruction(instruction.larg) && ir.get_assigned_register(instruction.larg) == Register::RAX && ir.has_death_point(instruction.larg, instruction.instruction_number)) {
        result += "call write\n";
    } else if (!ir.is_const_instruction(instruction.larg) && ir.get_assigned_register(instruction.larg) == Register::RAX) {
        result += "push %rax\ncall write\npop %rax\n";
    } else {
        result += std::format("push %rax\nmov {}, %rax\ncall write\npop %rax\n", reg_str(instruction.larg));
    }
    return result + "pop %rsi\npop %rdi\npop %rdx\npop %rcx\npop %rbx\n";
}

std::string CodeEmitter::read(const Instruction& instruction) {
    std::string result = "push %rax\npush %rdi\npush %rsi\npush %rdx\npush %rcx\n";
    result += "call read\nmov %rax, %r11\n";
    result += "pop %rcx\npop %rdx\npop %rsi\npop %rdi\npop %rax\n";
    return result + std::format("mov %r11, {}\n", reg_str(instruction.instruction_number));
}

std::string CodeEmitter::mov_instruction(const instruct_t& src, const instruct_t& dest) {
    std::string result = "";
    std::string src_str = reg_str(src);

    if(!ir.is_const_instruction(src) && !ir.is_const_instruction(dest) &&
       ir.get_assigned_register(src) == ir.get_assigned_register(dest))
    {
        return "";
    }
    
    if(is_virtual_reg(src) && is_virtual_reg(dest)) {
        result += std::format("mov {}, %r11\n", reg_str(src));
        src_str = "%r11";
    }
    
    if (!ir.is_const_instruction(dest))
    {
        return result + std::format("mov {}, {}\n", src_str, reg_str(dest));
    }

    std::cout << "error: invalid input to mov_instruction in codeemitter.cpp\n";
    return "";
}

/*
* Syntax - store in right register
* add <reg2>, <reg1>
* add <con>, <reg1>
*/
std::string CodeEmitter::additive_instruction(const instruct_t& left, const instruct_t& right, const std::string& operand)
{
    if(is_virtual_reg(left) && is_virtual_reg(right)) {
        return std::format("mov {}, %r11\n{} %r11, {}\n", reg_str(left), operand, reg_str(right)); 
    }
    return std::format("{} {}, {}\n", operand, reg_str(left), reg_str(right));
}

std::string CodeEmitter::additive(const Instruction& i, const std::string& operand) {
    if(reg_str(i.larg) == reg_str(i.instruction_number)) {
        return additive_instruction(i.rarg, i.instruction_number, operand);
    } else if(reg_str(i.rarg) == reg_str(i.instruction_number)){
        return additive_instruction(i.larg, i.instruction_number, operand);
    }
    return mov_instruction(i.larg, i.instruction_number) +
        additive_instruction(i.rarg, i.instruction_number, operand);
}


std::string CodeEmitter::scale(const Instruction& i, const std::string& operator_str) {
    std::string emit_string = "";
    emit_string += std::format(R"(push %rdx 
push %rax
push {}
push {}
mov $0, %rdx
mov 8(%rsp), %rax
{} (%rsp)
push %rax
add $8, %rsp
)", 
    reg_str(i.larg), reg_str(i.rarg), operator_str);

    if(!ir.is_const_instruction(i.rarg)) {
        emit_string += std::format("pop {}\n", reg_str(i.rarg));
    } else {
        emit_string += "add $8, %rsp\n";
    }
    if(!ir.is_const_instruction(i.larg)) {
        emit_string += std::format("pop {}\n", reg_str(i.larg));
    } else {
        emit_string += "add $8, %rsp\n";
    }
    emit_string += std::format(R"(pop %rax
pop %rdx
mov -40(%rsp), %r11
mov %r11, {}
)",
    reg_str(i.instruction_number));

    return emit_string;
}

std::string CodeEmitter::cmp(const Instruction& i) {
    std::string emit_string = "";

    std::string larg = reg_str(i.larg);

    std::string rarg = reg_str(i.rarg);

    if(ir.is_const_instruction(i.larg)){
        larg = ir.get_assigned_register(i.rarg) != Register::RAX ? "%rax" : "%rdx";
        emit_string += std::format("push {}\nmov {}, {}\n", larg, reg_str(i.larg), larg);
    }
    
    if(ir.is_const_instruction(i.rarg) || is_virtual_reg(i.rarg)){
        rarg = "%r11";
        emit_string += std::format("mov {}, {}\n", reg_str(i.rarg), rarg);
    }

    emit_string += "cmp " + rarg + ", " + larg + "\n";

    if(ir.is_const_instruction(i.larg))
        emit_string += "pop " + larg + "\n";

    return emit_string;
}

std::string CodeEmitter::prologue() {
    if(getting_pars) {
        getting_pars = false;
        return std::format(R"(mov %rsp, %rbp
mov %r11, %rsp
mov %rbp, %r11
mov 104(%rsp), %rbp
mov %r11, 104(%rsp)
push %rbp
mov %rsp, %rbp
add ${}, %rsp 
)", -8 * ir.spill_count);
    }
    return "";
}

std::string CodeEmitter::instruction(const Instruction& i) {
    std::string additional_instructions = "";
    switch(i.opcode) {
        case(Opcode::ADD):
            return prologue() + additive(i, "add");
        case(Opcode::SUB):
            return prologue() + additive(i, "sub");
        case(Opcode::MUL):
            return prologue() + scale(i, "mul");
        case(Opcode::DIV):
            return prologue() + scale(i, "div");
        case(Opcode::CMP):
            return prologue() + cmp(i);
        case(Opcode::BRA):
            return prologue() + branch(i.larg, "jmp");
        case(Opcode::BNE):
            return prologue() + branch(i.rarg, "jne");
        case(Opcode::BEQ):
            return prologue() + branch(i.rarg, "je");
        case(Opcode::BLE):
            return prologue() + branch(i.rarg, "jle");
        case(Opcode::BLT):
            return prologue() + branch(i.rarg, "jl");
        case(Opcode::BGE):
            return prologue() + branch(i.rarg, "jge");
        case(Opcode::BGT):
            return prologue() + branch(i.rarg, "jg");
        case(Opcode::JSR):
            additional_instructions = set_par_str;
            set_par_str = "";
            return prologue() + std::format(R"({}
call function{}
add $-120, %rsp
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
mov %rax, {}
{}
pop %r11
mov %r11, %rsp
)", additional_instructions, i.larg, reg_str(i.instruction_number), ir.get_assigned_register(i.instruction_number) == Register::RAX ? "add $8, %rsp" : "popq %rax");
        case(Opcode::RET):
            return prologue() + std::format(R"(add ${}, %rsp
pop %rbp
add $112, %rsp
{}
ret
)", 8 * ir.spill_count, i.larg != -1 ? std::format("mov {}, %rax", reg_str(i.larg)) : ""); 
        case(Opcode::MOV):
            return prologue() + mov_instruction(i.rarg, i.larg);
        case(Opcode::SWAP):
            return prologue() + std::format("mov {}, %r11\nmov {}, {}\nmov %r11, {}\n", reg_str(i.larg), reg_str(i.rarg), reg_str(i.larg), reg_str(i.rarg));
        case(Opcode::GETPAR):
            return std::format("pop {}\n", reg_str(i.instruction_number));
        case(Opcode::SETPAR):
            set_par_str += std::format("push {}\n", reg_str(i.larg));
            return prologue() + "";
        case(Opcode::READ):
            return prologue() + read(i);
        case(Opcode::WRITE):
            return prologue() + write(i);
        case(Opcode::WRITENL):
            return prologue() + 
R"(push %rax
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
)";
        default:
            return "";
    }
    return "";
}

std::string CodeEmitter::reg_str(const instruct_t& instruct) {
    if(ir.is_undefined_instruction(instruct )) return "$0";
    if(ir.is_const_instruction(instruct)) return std::format("${}", ir.get_const_value(instruct));
    if(is_virtual_reg(instruct)) return std::format("-{}(%rbp)", virtual_reg_offset(instruct));
    return reg_str_list.at(ir.get_assigned_register(instruct));
}

bool CodeEmitter::is_virtual_reg(const instruct_t& instruct) {
    return !ir.is_const_instruction(instruct) && ir.get_assigned_register(instruct) >= Register::UNASSIGNED;
}

int CodeEmitter::virtual_reg_offset(const instruct_t& instruct) {
  if(!is_virtual_reg(instruct)) throw std::runtime_error("This is not a virtual register!");
  return (ir.get_assigned_register(instruct) - Register::UNASSIGNED) * 8;
}
