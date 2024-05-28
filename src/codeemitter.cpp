#include "codeemitter.hpp"
#include <format>
#include <iostream>

CodeEmitter::CodeEmitter(IntermediateRepresentation&& ir) : ir(std::move(ir)), ofile("a.out") {}

void CodeEmitter::debug() const {
    ir.debug();
}

void CodeEmitter::emit_code() {
    static const std::string data_section = 
R"(section .data
    newline db 10
    newline_len equ 1
    digitSpace resb 100
    digitSpacePos resb 8
    buff resb 11

)";

    static const std::string default_text_section = 
R"(section .text
global _start
write:
    mov rcx, digitSpace
    mov rbx, 10
    mov [rcx], rbx
    ; inc rcx
    mov [digitSpacePos], rcx

_write_loop:
    mov rdx, 0
    mov rbx, 10
    div rbx
    push rax
    add rdx, 48
 
    mov rcx, [digitSpacePos]
    mov [rcx], dl
    inc rcx
    mov [digitSpacePos], rcx
    
    pop rax
    cmp rax, 0
    jne _write_loop

_write_loop2:
    mov rcx, [digitSpacePos]
 
    mov rax, 1
    mov rdi, 1
    mov rsi, rcx
    mov rdx, 1
    syscall
 
    mov rcx, [digitSpacePos]
    dec rcx
    mov [digitSpacePos], rcx

    cmp rcx, digitSpace
    jge _write_loop2

    cld                   ; Clear direction flag (to increment destination pointer)
    mov rcx, 100          ; Set the count to 100 bytes
    mov rdi, digitSpace   ; Set the destination pointer to digitSpace

    mov al, 0             ; Set the value to be stored (0 in this case)
    rep stosb             ; Store AL (0) in memory, incrementing destination pointer (EDI), repeat ECX times

    ret
read:
    ; Syscall to read input
    mov rax, 0            ; sys_read
    mov rdi, 0            ; file descriptor 0 (stdin)
    mov rsi, buff         ; buffer to store input
    mov rdx, 11           ; maximum number of bytes to read
    syscall               ; interrupt to call kernel

    ; Convert input string to integer
    mov rsi, buff         ; RSI points to the input buffer
    xor rax, rax          ; Clear RAX (result)
    xor rdi, rdi          ; Clear RDI (multiplier)

_read_loop:
    movzx rcx, byte [rsi]  ; Load current byte into RCX
    cmp rcx, 0x0A          ; Check for newline character
    je _read_done          ; If newline, we're done
    sub rcx, '0'           ; Convert ASCII to integer
    imul rax, rax, 10      ; Multiply current result by 10
    add rax, rcx           ; Add the current digit to the result
    inc rsi                ; Move to the next character
    jmp _read_loop         ; Repeat for next character

_read_done:
    ret

_start:

)";
    static const std::string exit = 
R"(mov rax, 60
mov rdi, 0
syscall
)"; 
   std::string program_string;

    // Emit main blocks
    for(const auto& block : ir.get_basic_blocks()) {
        if(!(block.index >= ir.get_successors(0).back())) continue;
        program_string += std::format("\n; BB{}\n", block.index);
        program_string += emit_block(block.index);
    }

    // Emit function blocks
    for(size_t index = 0; index < ir.get_successors(0).size() - 1; ++index) {
        program_string += std::format("function{}:\n", ir.get_instructions(ir.get_successors(0).at(index)).at(0).instruction_number);
        for(bb_t func_index = ir.get_successors(0).at(index); func_index < ir.get_successors(0).at(index+1); ++func_index) {
            program_string += std::format("\n; BB{}\n", func_index);
            program_string += emit_block(func_index);
        }
    }
    ofile << data_section << default_text_section << program_string << exit; 
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
    if(ir.get_assigned_register(i.instruction_number) == ir.get_assigned_register(i.larg)) {       
         return std::format("{} {}, {}\n", opcode, reg_str(i.larg), reg_str(i.rarg));
    }
    return std::format("mov {}, {}\n{} {}, {}\n", reg_str(i.instruction_number), reg_str(i.larg), opcode, reg_str(i.instruction_number), reg_str(i.rarg));
}

std::string CodeEmitter::emit_branch(const instruct_t& i, const std::string& opcode) {
    return std::format("{} branch{}\n", opcode, i);
}

std::string CodeEmitter::emit_write(const Instruction& instruction) {
    std::string result = "push rbx\npush rcx\npush rdx\npush rdi\npush rsi\n";
    if(!ir.is_const_instruction(instruction.larg) && ir.get_assigned_register(instruction.larg) == Register::RAX && ir.has_death_point(instruction.larg, instruction.instruction_number)) {
        result += "call write\n";
    } else if (!ir.is_const_instruction(instruction.larg) && ir.get_assigned_register(instruction.larg) == Register::RAX) {
        result += "push rax\ncall write\npop rax\n";
    } else {
        result += std::format("push rax\nmov rax, {}\ncall write\npop rax\n", reg_str(instruction.larg));
    }
    return result + "pop rsi\npop rdi\npop rdx\npop rcx\npop rbx\n";
}

std::string CodeEmitter::emit_read(const Instruction& instruction) {
    std::string result = "push rdi\npush rsi\npush rdx\npush rcx\n";
    if(ir.get_assigned_register(instruction.instruction_number) == Register::RAX) {
        result += "call read\n";
    } else {
        result += std::format("push rax\ncall read\nmov {}, rax\npop rax\n", reg_str(instruction.instruction_number));
    }
    return result + "pop rcx\npop rdx\npop rsi\npop rdi\n";

}

std::string CodeEmitter::emit_mov(const Instruction& instruction) {
    if(ir.is_const_instruction(instruction.rarg) || ir.get_assigned_register(instruction.larg) != ir.get_assigned_register(instruction.rarg)) {
        return std::format("mov {}, {}\n", reg_str(instruction.larg), reg_str(instruction.rarg));
    }
    return "";
}

std::string CodeEmitter::emit_instruction(const Instruction& i) {
    switch(i.opcode) {
        case(Opcode::ADD):
            return emit_basic(i, "add");
        case(Opcode::SUB):
            return emit_basic(i, "sub");
        case(Opcode::MUL):
            throw std::runtime_error("mul not implemented yet.");
            // return emit_basic(i, "mul");
        case(Opcode::DIV):
            return emit_basic(i, "div");
        case(Opcode::CMP):
            return std::format("cmp {}, {}\n", reg_str(i.larg), reg_str(i.rarg));
        case(Opcode::PHI):
            return "";
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
            return emit_mov(i);
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
            R"(push rax\n
            push rdi\n
            push rsi\n
            push rdx\n
            mov rax, 1\n
            mov rdi, 1\n
            mov rsi, newline\n
            mov rdx, newline_len\n
            syscall\n
            pop rdx\n
            pop rsi\n
            pop rdi\n
            pop rax\n)";
        default:
            return "";
    }
    return "";
}

std::string CodeEmitter::reg_str(const instruct_t& instruct) {
    if(ir.is_undefined_instruction(instruct )) return "0";
    if(ir.is_const_instruction(instruct)) return std::format("{}", ir.get_const_value(instruct));
    return reg_str_list.at(ir.get_assigned_register(instruct));
}
