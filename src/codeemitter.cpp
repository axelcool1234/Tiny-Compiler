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

)";

    static const std::string default_text_section = 
R"(.section .text
.global _start
write:
    movabsq $10, %rcx       # divisor
    xorl %ebx, %ebx         # count digits

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
R"(movq $60, %rax    # System call number for exit
movq $0, %rdi     # Exit code 0
syscall
)"; 
   std::string program_string;

    // mov rsp rbp // TODO
    // sub {spill_count * 8}, rsp // TODO

    // Emit main blocks
    for(const auto& block : ir.get_basic_blocks()) {
        if(!(block.index >= ir.get_successors(0).back())) continue;
        program_string += std::format("\n# BB{}\n", block.index);
        program_string += emit_block(block.index);
    }

    // add, rsp, {spill_count * 8} // emit when we run into a return //TODO

    // Emit function blocks
    for(size_t index = 0; index < ir.get_successors(0).size() - 1; ++index) {
        program_string += std::format("function{}:\n", ir.get_instructions(ir.get_successors(0).at(index)).at(0).instruction_number);
        for(bb_t func_index = ir.get_successors(0).at(index); func_index < ir.get_successors(0).at(index+1); ++func_index) {
            program_string += std::format("\n# BB{}\n", func_index);
            program_string += emit_block(func_index);
        }
    }
    ofile << default_text_section << program_string << data_section << exit; 
}

std::string CodeEmitter::emit_block(const bb_t& b) {
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
            block_string += emit_instruction(instruct);
        }
    }

    // Emit the branch instruction of the given block.
    if(ir.has_branch_instruction(b)) {
        block_string += emit_instruction(ir.get_branch_instruction(b));
    }

    // Block is now emitted.
    ir.set_emitted(b);

    return block_string;
}

std::string CodeEmitter::emit_basic(const Instruction& i, const std::string& opcode) {
    if(ir.is_const_instruction(i.larg) || ir.is_const_instruction(i.rarg)) {  
        return std::format("movq ${}, {}\n{} ${}, {}\n", reg_str(i.larg), reg_str(i.instruction_number), opcode, reg_str(i.rarg), reg_str(i.instruction_number));
    }
    if(ir.get_assigned_register(i.instruction_number) == ir.get_assigned_register(i.larg)) {    
        return std::format("{} {}, {}\n", opcode, reg_str(i.larg), reg_str(i.rarg));
    }
    return std::format("mov {}, {}\n{} {}, {}\n", reg_str(i.instruction_number), reg_str(i.larg), opcode, reg_str(i.rarg), reg_str(i.instruction_number));
}

std::string CodeEmitter::emit_branch(const instruct_t& i, const std::string& opcode) {
    return std::format("{} branch{}\n", opcode, i);
}

std::string CodeEmitter::emit_write(const Instruction& instruction) {
    std::string result = "push %rbx\npush %rcx\npush %rdx\npush %rdi\npush %rsi\n";
    if(!ir.is_const_instruction(instruction.larg) && ir.get_assigned_register(instruction.larg) == Register::RAX && ir.has_death_point(instruction.larg, instruction.instruction_number)) {
        result += "call write\n";
    } else if (!ir.is_const_instruction(instruction.larg) && ir.get_assigned_register(instruction.larg) == Register::RAX) {
        result += "push %rax\ncall write\npop %rax\n";
    } else {
        result += std::format("push %rax\nmov %rax, {}\ncall write\npop %rax\n", reg_str(instruction.larg));
    }
    return result + "pop %rsi\npop %rdi\npop %rdx\npop %rcx\npop %rbx\n";
}

std::string CodeEmitter::emit_read(const Instruction& instruction) {
    std::string result = "push %rdi\npush %rsi\npush %rdx\npush %rcx\n";
    if(ir.get_assigned_register(instruction.instruction_number) == Register::RAX) {
        result += "call read\n";
    } else {
        result += std::format("push %rax\ncall read\nmov {}, %rax\npop %rax\n", reg_str(instruction.instruction_number));
    }
    return result + "pop %rcx\npop %rdx\npop %rsi\npop %rdi\n";

}

std::string CodeEmitter::emit_mov(const Instruction& instruction) {
    if(ir.is_const_instruction(instruction.rarg) || ir.get_assigned_register(instruction.larg) != ir.get_assigned_register(instruction.rarg)) {
        return std::format("mov {}, {}\n", reg_str(instruction.larg), reg_str(instruction.rarg));
    }
    return "";
}

std::string CodeEmitter::mov_instruction(const instruct_t& from, const instruct_t& to) {
    if (!ir.is_const_instruction(from) && !ir.is_const_instruction(to))
    {
        if(reg_str(from) == reg_str(to))
        {
            return "";
        }
        else
            return std::format("mov {}, {}\n", reg_str(from), reg_str(to));
    }
   
    else if(ir.is_const_instruction(from) && !ir.is_const_instruction(to))
    {
        return std::format("mov ${}, {}\n", reg_str(from), reg_str(to));
    }

    std::cout << "Error: Invalid input to mov_instruction in codeemitter.cpp\n";
    return "";
}

// mov_register(i.larg, Register::RAX)

std::string CodeEmitter::mov_register(const instruct_t& from, Register to)
{
    if(!ir.is_const_instruction(from))
    {
        if (reg_str(from) == reg_str_list[to])
            return "";
        else
            return std::format("mov {}, {}\n", reg_str(from), reg_str_list[to]);
    }

    else if (ir.is_const_instruction(from))
    {
        return std::format("mov ${}, {}\n", reg_str(from), reg_str_list[to]);
    }

    std::cout << "Error: Invalid input to mov_instruction in codeemitter.cpp\n";
    return "";
}

std::string CodeEmitter::mov_register(Register from, const instruct_t& to)
{
    if(!ir.is_const_instruction(to))
    {
        if (reg_str(to) == reg_str_list[from])
            return "";
        else
            return std::format("mov {}, {}\n", reg_str_list[from], reg_str(to));
    }
    // else if (!ir.is_const_instruction(to))
    // {
    //     return std::format("mov ${}, {}\n", reg_str_list[from], reg_str(to));
    // }

    std::cout << "Error: Invalid input to mov_instruction in codeemitter.cpp\n";
    return "";
}

/*
* Syntax - store in right register
* add <reg2>, <reg1>
* add <con>, <reg1>
*/
std::string CodeEmitter::add_instruction(const instruct_t& left, const instruct_t& right)
{
    if (!ir.is_const_instruction(left) && !ir.is_const_instruction(right))
    {
        return std::format("add {}, {}\n", reg_str(left), reg_str(right));
    }
   
    if(ir.is_const_instruction(left) && !ir.is_const_instruction(right))
    {
        return std::format("add ${}, {}\n", reg_str(left), reg_str(right));
    }

    std::cout << "Error: Invalid input to add_instruction in codeemitter.cpp\n";
    return "";
}

/*
* Syntax - store in right register
* sub <reg2>, <reg1>
* sub <con>, <reg1>
*/
std::string CodeEmitter::sub_instruction(const instruct_t& left, const instruct_t& right)
{
    if (!ir.is_const_instruction(left) && !ir.is_const_instruction(right))
    {
        return std::format("sub {}, {}\n", reg_str(left), reg_str(right));
    }
   
    if(ir.is_const_instruction(left) && !ir.is_const_instruction(right))
    {
        return std::format("sub ${}, {}\n", reg_str(left), reg_str(right));
    }

    std::cout << "Error: Invalid input to sub_instruction in codeemitter.cpp\n";
    return "";
}

std::string CodeEmitter::div_instruction(const instruct_t& i)
{
    if(!ir.is_const_instruction(i)) {
        return std::format("div {}\n", reg_str(i));
    }
    std::cout << "Error: Invalid input to div_instruction in codeemitter.cpp\n";
    return "";
}

std::string CodeEmitter::div_register(Register reg)
{
    return std::format("div {}\n", reg_str_list[reg]);
}

// push <reg64>
std::string CodeEmitter::push_instruction(const instruct_t& i) {
    if (!ir.is_const_instruction(i)) {
        return std::format("push {}\n", reg_str(i));
    }
    std::cout << "Error: Invalid input to push_instruction in codeemitter.cpp\n";
    return "";
}

//override push for enum
std::string CodeEmitter::push_register(Register reg)
{
    return std::format("push {}\n", reg_str_list[reg]);
}

// pop <reg64>
std::string CodeEmitter::pop_instruction(const instruct_t& i) {
    if (!ir.is_const_instruction(i)) {
        return std::format("pop {}\n", reg_str(i));
    }
    std::cout << "Error: Invalid input to pop_instruction in codeemitter.cpp\n";
    return "";
}

//override pop for enum
std::string CodeEmitter::pop_register(Register reg)
{
    return std::format("pop {}\n", reg_str_list[reg]);
}

std::string CodeEmitter::add_emitter(const Instruction& i) {
    if(reg_str(i.larg) == reg_str(i.instruction_number)) {
        return add_instruction(i.rarg, i.instruction_number);
    } else if(reg_str(i.rarg) == reg_str(i.instruction_number)){
        return add_instruction(i.larg, i.instruction_number);
    }
    return mov_instruction(i.larg, i.instruction_number) +
        add_instruction(i.rarg, i.instruction_number);
}

std::string CodeEmitter::sub_emitter(const Instruction& i) {
    if(!ir.is_const_instruction(i.rarg)){
        return std::format("neg {}", reg_str(i.rarg)) + add_emitter(i);
    }
    return mov_instruction(i.larg, i.instruction_number) + 
        sub_instruction(i.rarg, i.instruction_number);
}

// push rdx
// push rax

// mov i.larg rax
// idiv i.rarg
// mov rax i.instruction_number

// pop rax
// pop rdx
std::string CodeEmitter::div_emitter(const Instruction& i) {
    std::string emit_string;
    if (reg_str(i.instruction_number) != reg_str_list[Register::RDX])
        emit_string += push_register(Register::RDX);
    if (reg_str(i.instruction_number) != reg_str_list[Register::RAX])
        emit_string += push_register(Register::RAX);
    
    emit_string += mov_register(i.larg, Register::RAX);
    if (ir.is_const_instruction(i.rarg)) {
        emit_string += mov_register(i.rarg, Register::RDX);
        emit_string += div_register(Register::RDX); // TODO Ilya
    } else {
        emit_string += div_instruction(i.rarg);
    }
    emit_string += mov_register(Register::RAX, i.instruction_number);

    if (reg_str(i.instruction_number) != reg_str_list[Register::RAX])
        emit_string += pop_instruction(Register::RAX);
    if (reg_str(i.instruction_number) != reg_str_list[Register::RDX])
        emit_string += pop_instruction(Register::RDX);
    return emit_string;
}


std::string CodeEmitter::emit_instruction(const Instruction& i) {
    switch(i.opcode) {
        case(Opcode::ADD):
            return add_emitter(i);
        case(Opcode::SUB):
            return sub_emitter(i);
        case(Opcode::MUL):
        // push rdx         // Store rdx

        // mov i.larg rax   // one of the arguments into rax
        // mul i.rarg      // multiply rax by i.rarg
        // mov rax i.instruction_number     //move the result of the multiplication into i.instruction number.
        
        // pop rdx          // repopulate  rdx
        return("");
        case(Opcode::DIV):
            return div_emitter(i);
        case(Opcode::CMP):
            return std::format("cmp {}, {}\n", reg_str(i.larg), reg_str(i.rarg));
        case(Opcode::BRA):
            return emit_branch(i.larg, "jmp");
        case(Opcode::BNE):
            return emit_branch(i.rarg, "jne");
        case(Opcode::BEQ):
            return emit_branch(i.rarg, "je");
        case(Opcode::BLE):
            return emit_branch(i.rarg, "jle");
        case(Opcode::BLT):
            return emit_branch(i.rarg, "jl");
        case(Opcode::BGE):
            return emit_branch(i.rarg, "jge");
        case(Opcode::BGT):
            return emit_branch(i.rarg, "jg");
        case(Opcode::JSR):
            throw std::runtime_error("Functions not implemented yet!");
            break;
        case(Opcode::RET):
            throw std::runtime_error("Function returns not implemented yet!");
            break;
        case(Opcode::MOV):
            return mov_instruction(i.rarg, i.larg);
            break;
        case(Opcode::SWAP):
            return std::format("xchg {}, {}\n", reg_str(i.larg), reg_str(i.rarg));
        case(Opcode::GETPAR):
            throw std::runtime_error("Function GETPAR not implemented yet!");
            break;
        case(Opcode::SETPAR):
            throw std::runtime_error("Function SETPAR not implemented yet!");
            break;
        case(Opcode::READ):
            return emit_read(i);
        case(Opcode::WRITE):
            return emit_write(i);
        case(Opcode::WRITENL):
            return 
            R"(push %rax\n
            push %rdi\n
            push %rsi\n
            push %rdx\n
            mov rax, $1\n
            mov rdi, $1\n
            mov rsi, newline\n
            mov rdx, newline_len\n
            syscall\n
            pop %rdx\n
            pop %rsi\n
            pop %rdi\n
            pop %rax\n)";
        default:
            return "";
    }
    return "";
}

std::string CodeEmitter::reg_str(const instruct_t& instruct) {
    if(ir.is_undefined_instruction(instruct )) return "0";
    if(ir.is_const_instruction(instruct)) return std::format("{}", ir.get_const_value(instruct));
    if(is_virtual_reg(instruct)) return std::format("-{}(%rbp)", virtual_reg_offset(instruct));
    return reg_str_list.at(ir.get_assigned_register(instruct));
}

bool CodeEmitter::is_virtual_reg(const instruct_t& instruct) {
  return ir.get_assigned_register(instruct) >= Register::UNASSIGNED;
}

int CodeEmitter::virtual_reg_offset(const instruct_t& instruct) {
  if(!is_virtual_reg(instruct)) throw std::runtime_error("This is not a virtual register!");
  return (ir.get_assigned_register(instruct) - Register::UNASSIGNED) * 8;
}
