#include "registerallocator.hpp"
#include <format>
#include <ranges>
#include <iostream>
#include <unordered_set>
#include <stdexcept>

RegisterAllocator::RegisterAllocator(IntermediateRepresentation&& ir) : ir(std::move(ir)), ofile("a.out") {}

void RegisterAllocator::allocate_registers() {
    get_const_instructions();
    liveness_analysis();
    color_ir();
}

void RegisterAllocator::emit_code() {
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
    for(const auto& block : ir.basic_blocks) {
        if(!(block.index >= ir.basic_blocks[ir.basic_blocks[0].successors.back()].index)) continue;
        program_string += std::format("\n; BB{}\n", block.index);
        program_string += emit_block(block.index);
    }

    // Emit function blocks
    for(size_t index = 0; index < ir.basic_blocks[0].successors.size() - 1; ++index) {
        program_string += std::format("function{}:\n", ir.basic_blocks[ir.basic_blocks[0].successors[index]].instructions[0].instruction_number);
        for(bb_t func_index = ir.basic_blocks[0].successors[index]; func_index < ir.basic_blocks[0].successors[index+1]; ++func_index) {
            program_string += std::format("\n; BB{}\n", func_index);
            program_string += emit_block(func_index);
        }
    }
    // program_string += emit_scope(ir.basic_blocks[0].successors.back()); // Emitting main function
    // for(const auto& block : ir.basic_blocks[0].successors | std::views::reverse | std::views::drop(1)) {
        // throw std::runtime_error("Functions not implemented yet!");
        // program_string += emit_function(block); // Emitting user defined functions
    // }
    ofile << data_section << default_text_section << program_string << exit; 
}

std::string RegisterAllocator::emit_function(const bb_t& b) {
    std::string function_string;
    if(ir.basic_blocks[b].instructions.size() != 0) {
        function_string += std::format("function{}:\n", ir.basic_blocks[b].instructions[0].instruction_number);
    }
    function_string += emit_scope(b) + "ret\n";
    return function_string;
}

std::string RegisterAllocator::emit_scope(const bb_t& b) {
    std::string scope_string;
    scope_string += emit_block(b);
    const std::vector<bb_t>& successors = ir.basic_blocks[b].successors;
    if(successors.size() == 0) {
        return "";
    } else if(successors.size() == 1) {
        scope_string += emit_scope(successors[0]);
    } else if(ir.basic_blocks[successors[0]].type == Blocktype::IF_FALLTHROUGH || ir.basic_blocks[successors[0]].type == Blocktype::WHILE_FALLTHROUGH) {
        scope_string += emit_scope(successors[0]);
        scope_string += emit_scope(successors[1]);
    } else {
        scope_string += emit_scope(successors[1]);
        scope_string += emit_scope(successors[0]);
    }
    return scope_string;
}

std::string RegisterAllocator::emit_block(const bb_t& b) {
    std::string block_string;
    BasicBlock& block = ir.basic_blocks.at(b);
    if(block.emitted) return "";
    if(block.predecessors.size() == 2 && (!ir.basic_blocks[block.predecessors[0]].emitted || !ir.basic_blocks[block.predecessors[1]].emitted)) return "";
    if(block.instructions.size() != 0) {
        if(block.type == Blocktype::JOIN || block.type == Blocktype::IF_BRANCH || block.type == Blocktype::WHILE_BRANCH || block.branch_block != -1 || block.loop_header != -1) {
            block_string += std::format("branch{}:\n", block.instructions[0].instruction_number);
        }
        for(const auto& instruct : block.instructions) {
            block_string += emit_instruction(instruct);
        }
    }
    if(block.branch_instruction.opcode != Opcode::EMPTY) {
        block_string += emit_instruction(block.branch_instruction);
    }
    block.emitted = true;
    return block_string;
}

std::string RegisterAllocator::emit_basic(const Instruction& i, const std::string& opcode) {
    if(assigned_registers[i.instruction_number] == assigned_registers[i.larg]) {
        return std::format("{} {}, {}\n", opcode, reg_str(i.larg), reg_str(i.rarg));
    }
    return std::format("mov {}, {}\n{} {}, {}\n", reg_str(i.instruction_number), reg_str(i.larg), opcode, reg_str(i.instruction_number), reg_str(i.rarg));
}

std::string RegisterAllocator::emit_branch(const instruct_t& i, const std::string& opcode) {
    return std::format("{} branch{}\n", opcode, i);
}
std::string RegisterAllocator::emit_instruction(const Instruction& i) {
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
            return std::format("mov {}, {}\n", reg_str(i.larg), reg_str(i.rarg));
            break;
        case(Opcode::GETPAR):
            throw std::runtime_error("Function GETPAR not implemented yet!");
            break;
        case(Opcode::SETPAR):
            throw std::runtime_error("Function SETPAR not implemented yet!");
            break;
        case(Opcode::READ):
            return std::format("call read\nmov {}, eax\n", reg_str(i.instruction_number));
        case(Opcode::WRITE):
            return std::format("mov eax, {}\ncall write\n", reg_str(i.instruction_number));
        case(Opcode::WRITENL):
            return "mov rax, 1\nmov rdi, 1\nmov rsi, newline\nmov rdx, newline_len\nsyscall\n";
        default:
            return "";
    }
    return "";
}

std::string RegisterAllocator::reg_str(const instruct_t& instruction) {
    if(const_instructions.find(instruction) != const_instructions.end()) return std::format("{}", const_instructions_map[instruction]);
    if(instruction == 0) return "0";
    return reg_str_list[assigned_registers[instruction]];
}

void RegisterAllocator::get_const_instructions() {
    for(const auto& instruction : ir.basic_blocks[0].instructions) {
        const_instructions.insert(instruction.instruction_number);
        const_instructions_map[instruction.instruction_number] = instruction.larg;
    }
}

/* Liveness Analysis */
void RegisterAllocator::liveness_analysis() {
    for(auto& block : ir.basic_blocks) {
        if(block.successors.size() == 0) {
            analyze_block(block);
        }
    }
}

void RegisterAllocator::analyze_block(BasicBlock& block) {
    // Ensure block hasn't been analyzed yet.
    // If the block in question is the const block, skip it.
    if(block.analyzed || block.index == 0) return;

    // Ensure everything before the block has been analyzed first.
    for(const auto& successor : block.successors) {
        if(!ir.basic_blocks[successor].analyzed) return;
    }

    // Grab live SSA instructions from successors.
    for(const auto& successor : block.successors) {
        live_ins[block.index].insert(live_ins[successor].begin(), live_ins[successor].end());
    }

    // Calculate deaths of instructions in future shuffle code
    // This is arguably unsafe due to it being a consequence of my parser implementation.
    //
    // Because of how phi functions are created in the parser, right arguments always
    // belong to the non-branching block, left arguments belong to branching blocks.
    //
    // Left arguments also belong to blocks above the phi_block in the case the phi_block is a
    // loop header.
    //
    // Right arguments also belong to branch-back blocks from the phi_block in the case the
    // phi_block is a loop header.

    // If we've propagated the phis already:
    // if(block.propagated) return;

    // Get liveness from loop header (that you're re-entering since you're already in the loop)
    if(block.loop_header != -1) {
        for(const auto& instruction : ir.basic_blocks[block.loop_header].instructions) {
            if(instruction.opcode != Opcode::PHI) break;
            if(const_instructions.find(instruction.rarg) != const_instructions.end()) continue;
            live_ins[block.index].insert(instruction.rarg);
        }
    } 
    // Get liveness from loop header (that you're entering for first time)
    else if(block.successors.size() == 1 && ir.basic_blocks[block.successors[0]].branch_block != -1) {
        for(const auto& instruction : ir.basic_blocks[block.successors[0]].instructions) {
            if(instruction.opcode != Opcode::PHI) break;
            if(const_instructions.find(instruction.larg) != const_instructions.end()) continue;
            live_ins[block.index].insert(instruction.larg);
        }
    }
    // Get liveness from JOIN block (as block branching to it)
    else if(block.successors.size() == 1 && block.branch_instruction.opcode != Opcode::EMPTY) {
        for(const auto& instruction : ir.basic_blocks[block.successors[0]].instructions) {
            if(instruction.opcode != Opcode::PHI) break;
            if(const_instructions.find(instruction.larg) != const_instructions.end()) continue;
            live_ins[block.index].insert(instruction.larg);
        }
    } 
    // Get liveness from JOIN block (as block falling through to it)
    else if(block.successors.size() == 1) {
        for(const auto& instruction : ir.basic_blocks[block.successors[0]].instructions) {
            if(instruction.opcode != Opcode::PHI) break;
            if(const_instructions.find(instruction.rarg) != const_instructions.end()) continue;
            live_ins[block.index].insert(instruction.rarg);
        }
    } 
    
    // Determine points of death and liveness of SSA instructions at the beginning of the block.
    for(const auto& instruction : block.instructions | std::views::reverse) {
        // When something is born, everything beyond this point will not have it as a living SSA instruction. 
        live_ins[block.index].erase(instruction.instruction_number);

        // Ignore arguments of phi instructions since the move/swap calls will be in the predecessors.
        // Also ignore branch instructions as they don't require registers to work correctly in x86.
        if(instruction.opcode == Opcode::PHI ||
           instruction.opcode == Opcode::BRA ||
           instruction.opcode == Opcode::BNE ||
           instruction.opcode == Opcode::BEQ ||
           instruction.opcode == Opcode::BLE ||
           instruction.opcode == Opcode::BLT ||
           instruction.opcode == Opcode::BGE ||
           instruction.opcode == Opcode::BGT ||
           instruction.opcode == Opcode::EMPTY) {
             continue;
        }

        // When something dies, everything beyond this point will have it as a living SSA instruction.
        if(instruction.larg != -1 && const_instructions.find(instruction.larg) == const_instructions.end() 
                                  && live_ins[block.index].find(instruction.larg) == live_ins[block.index].end()) {
            live_ins[block.index].insert(instruction.larg);
            death_points[instruction.larg].insert(instruction.instruction_number);
        }
        if(instruction.rarg != -1 && const_instructions.find(instruction.rarg) == const_instructions.end() 
                                  && live_ins[block.index].find(instruction.rarg) == live_ins[block.index].end()) {
            live_ins[block.index].insert(instruction.rarg);
            death_points[instruction.rarg].insert(instruction.instruction_number);
        }
    }

    // This block has now been analyzed.
    block.analyzed = true;

    // Analyze predecessor blocks.
    for(const auto& predecessor : block.predecessors) {
        analyze_block(ir.basic_blocks[predecessor]);
    }
}

/* Graph Coloring */
void RegisterAllocator::color_ir() {
    // No need to color the const block.
    ir.basic_blocks[0].processed = true;

    // Color every function in the program (including main).
    for(auto& start_block : ir.basic_blocks[0].successors) {
        color_block(ir.basic_blocks[start_block]);
    }
}

void RegisterAllocator::color_block(BasicBlock& block) {
    // Ensure immediate dominator has been colored first.
    if(!ir.basic_blocks[ir.doms[block.index]].processed) return;

    // If block is already colored, skip.
    if(block.processed) return;

    // Start occupied register set as empty.
    std::unordered_set<Register> occupied{};

    // For SSA instructions that are live at the beginning of the block, make sure
    // their assigned registers are occupied.
    for(const instruct_t& live_instruct : live_ins[block.index]) {
        occupied.insert(assigned_registers[live_instruct]);
    }

    // For SSA phi instructions, get registers for them and ignore their arguments.
    // The arguments of the phi functions are ignored because they correspond to move 
    // instructions in the current block's predecessors and hence don't represent live 
    // instructions in the current block.
    for(const auto& instruction : block.instructions) {
        if(instruction.opcode != Opcode::PHI) break;
        assigned_registers[instruction.instruction_number] = get_register(instruction, occupied);
        occupied.insert(assigned_registers[instruction.instruction_number]);
    }

    // Assign registers
    for(const auto& instruction : block.instructions) {
        if(instruction.opcode == Opcode::PHI || instruction.opcode == Opcode::EMPTY) continue;

        // Unoccupy registers from dead SSA instructions
        if(instruction.larg != -1 && death_points[instruction.larg].find(instruction.instruction_number) != death_points[instruction.larg].end()) {
            occupied.erase(assigned_registers[instruction.larg]);
        }
        if(instruction.rarg != -1 && death_points[instruction.rarg].find(instruction.instruction_number) != death_points[instruction.rarg].end()) {
            occupied.erase(assigned_registers[instruction.rarg]);
        }

        // The following instructions do not need to be assigned a register
        if(instruction.opcode == Opcode::JSR ||
           instruction.opcode == Opcode::GETPAR ||
           instruction.opcode == Opcode::CMP ||
           instruction.opcode == Opcode::MOV ||
           instruction.opcode == Opcode::SWAP ||
           instruction.opcode == Opcode::WRITE ||
           instruction.opcode == Opcode::WRITENL ||
           instruction.opcode == Opcode::BRA ||
           instruction.opcode == Opcode::BNE ||
           instruction.opcode == Opcode::BEQ ||
           instruction.opcode == Opcode::BLE ||
           instruction.opcode == Opcode::BLT ||
           instruction.opcode == Opcode::BGE ||
           instruction.opcode == Opcode::BGT ||
           instruction.opcode == Opcode::EMPTY) {
             continue;
        }

        // Assign oneself a register
        assigned_registers[instruction.instruction_number] = get_register(instruction, occupied);
        occupied.insert(assigned_registers[instruction.instruction_number]);
    }

    // This block has now been colored
    block.processed = true;

    // Create phi moves where necessary
    if(block.loop_header != -1) { // current block is a branch-back block
        implement_phi_copies(block, ir.basic_blocks[block.loop_header]);
    }
    for(const auto& predecessor : block.predecessors) {
        if(ir.basic_blocks[predecessor].processed) {
            implement_phi_copies(ir.basic_blocks[predecessor], block);
        }
    }
    for(const auto& successor: block.successors) {
        if(ir.basic_blocks[successor].processed) {
            implement_phi_copies(block, ir.basic_blocks[successor]);
        }
    }

    // Color successors
    for(const auto& successor : block.successors) {
        color_block(ir.basic_blocks[successor]);
    }
}

void RegisterAllocator::implement_phi_copies(BasicBlock& block, BasicBlock& phi_block) {
    // This is arguably unsafe due to it being a consequence of my parser implementation.
    //
    // Because of how phi functions are created in the parser, right arguments always
    // belong to the non-branching block, left arguments belong to branching blocks.
    //
    // Left arguments also belong to blocks above the phi_block in the case the phi_block is a
    // loop header.
    //
    // Right arguments also belong to branch-back blocks from the phi_block in the case the
    // phi_block is a loop header.

    // If we've propagated the phis already:
    // if(block.propagated) return;

    // While loop header (phi_block) and branch-back block (block)
    if(block.loop_header == phi_block.index && phi_block.branch_block == block.index) {
        for(const auto& instruction : phi_block.instructions) {
            if(instruction.opcode != Opcode::PHI) break; 
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.rarg 
            ir.add_instruction(block.index, Opcode::MOV, instruction.instruction_number, instruction.rarg); 
        }
    } 
    // While loop header (phi_block) and block above it (block)
    else if(phi_block.branch_block != -1) {
        for(const auto& instruction : phi_block.instructions) {
            if(instruction.opcode != Opcode::PHI) break;
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.larg
            ir.add_instruction(block.index, Opcode::MOV, instruction.instruction_number, instruction.larg);
        }
    }
    // Join block (phi_block) and block branching to it (block)
    else if(block.branch_instruction.opcode != Opcode::EMPTY) {
        for(const auto& instruction : phi_block.instructions) {
            if(instruction.opcode != Opcode::PHI) break;
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.larg
            ir.add_instruction(block.index, Opcode::MOV, instruction.instruction_number, instruction.larg);
        }
    }
    // Join block (phi_block) and block falling through to it (block)
    else {
        for(const auto& instruction : phi_block.instructions) {
            if(instruction.opcode != Opcode::PHI) break;
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.rarg
            ir.add_instruction(block.index, Opcode::MOV, instruction.instruction_number, instruction.rarg);
        }
    }

    // Phis have been propagated to the block.
    // block.propagated = true;
}

Register RegisterAllocator::get_register(const Instruction& instruction, const std::unordered_set<Register>& occupied) {
    for(int reg = FIRST + 1; reg < Register::LAST; ++reg) {
        if(occupied.find(static_cast<Register>(reg)) == occupied.end()) return static_cast<Register>(reg);
    }
    throw std::runtime_error{"Ran out of registers"};
}

/* Debugging */
void RegisterAllocator::debug() const {
    print_const_instructions();
    print_leaf_blocks();
    print_live_ins();
    print_colored_instructions();
    print_death_points();
    print_unanalyzed_blocks();
    print_uncolored_blocks();
    print_unemitted_blocks();

    std::cout << "--- After Phi Propagation ---" << std::endl;
    std::cout << ir.to_dotlang();
}

void RegisterAllocator::print_const_instructions() const {
    std::cout << "--- Constant Instructions ---" << std::endl;
    for(const auto& instruction : const_instructions) {
        std::cout << "Instruction: " << instruction << std::endl;
    }
}

void RegisterAllocator::print_live_ins() const {
    std::cout << "--- Live-Ins ---" << std::endl;
    for(size_t block_index = 0; block_index < ir.basic_blocks.size(); ++block_index) {
        std::cout << std::format("Block {}: ", block_index);
        for(const auto& live : live_ins[block_index]) {
            std::cout << live << ", ";
        }
        std::cout << std::endl;
    }
}

void RegisterAllocator::print_leaf_blocks() const {
    std::cout << "--- Leaf Blocks ---" << std::endl;
    for(const auto& block : ir.basic_blocks) {
        if(block.successors.size() == 0) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void RegisterAllocator::print_unanalyzed_blocks() const {
    std::cout << "--- Unanalyzed Blocks ---" << std::endl;
    for(const auto& block : ir.basic_blocks) {
        if(!block.analyzed) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void RegisterAllocator::print_uncolored_blocks() const {
    std::cout << "--- Uncolored Blocks ---" << std::endl;
    for(const auto& block : ir.basic_blocks) {
        if(!block.processed) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void RegisterAllocator::print_unemitted_blocks() const {
    std::cout << "--- Unemitted Blocks ---" << std::endl;
    for(const auto& block : ir.basic_blocks) {
        if(!block.emitted) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void RegisterAllocator::print_colored_instructions() const {
    std::cout << "--- Colored Instructions ---" << std::endl;
    for(const auto& colored_instruction : assigned_registers) {
        std::cout << "Instruction: " << colored_instruction.first << " Register: " << colored_instruction.second << std::endl;
    }
}

void RegisterAllocator::print_death_points() const {
    std::cout << "--- Death Points ---" << std::endl;
    for(const auto& instruction_death: death_points) {
        std::cout << "Instruction: " << instruction_death.first << " Death: ";
        for(const auto& death : instruction_death.second) {
            std::cout << death << ", ";
        }
        std::cout << std::endl;
    }
}
