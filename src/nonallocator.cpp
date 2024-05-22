#include "nonallocator.hpp"
#include "intermediaterepresentation.hpp"
#include <format>
#include <iostream>
#include <ranges>

NonAllocator::NonAllocator(IntermediateRepresentation&& ir) : ir(std::move(ir)), ofile("a.out") {}

void NonAllocator::generate_data_section() {
    ofile << "section .data" << std::endl;
    for(const auto& instruct : ir.basic_blocks[0].instructions) {
        const_instructions.insert(instruct.instruction_number);
        ofile << std::format("const{} dd {}", instruct.instruction_number, instruct.larg) << std::endl;
    }
    for(int i = 0; i <= ir.instruction_count; ++i) {
        if(const_instructions.find(i) != const_instructions.end()) continue;
        ofile << std::format("nonconst{} dd 0", i) << std::endl;
    }
    ofile << "newline db 10" << std::endl;
    ofile << "newline_len equ 1" << std::endl;
    ofile << "digitSpace resb 100" << std::endl;
    ofile << "digitSpacePos resb 8" << std::endl;
    ofile << std::endl;
}

void NonAllocator::destroy_phis() {
    for(const auto& block : ir.basic_blocks) {
        for(const auto& instruct : block.instructions) {
            if (instruct.opcode != Opcode::PHI) continue;
            if (block.branch_block != -1) {
                 destroy_while_phi(block, instruct);
            }
            else {
                destroy_if_phi(block, instruct);
            }
        }
    }
}

void NonAllocator::destroy_while_phi(const BasicBlock& b, const Instruction& i) {
    const BasicBlock& pred = ir.basic_blocks[b.predecessors[0]];
    const BasicBlock& branch_block = ir.basic_blocks[b.branch_block];
    ir.add_instruction(pred.index, Opcode::MOV, i.instruction_number, i.larg);
    ir.add_instruction(branch_block.index, Opcode::MOV, i.instruction_number, i.rarg);
}

void NonAllocator::destroy_if_phi(const BasicBlock& b, const Instruction& i) {
    const BasicBlock& pred1 = ir.basic_blocks[b.predecessors[0]];
    const BasicBlock& pred2 = ir.basic_blocks[b.predecessors[1]];
    if (pred1.type == Blocktype::IF_FALLTHROUGH || pred2.type == Blocktype::WHILE_BRANCH) {
        ir.add_instruction(pred1.index, Opcode::MOV, i.instruction_number, i.larg);
        ir.add_instruction(pred2.index, Opcode::MOV, i.instruction_number, i.rarg);
    } else {
        ir.add_instruction(pred2.index, Opcode::MOV, i.instruction_number, i.larg);
        ir.add_instruction(pred1.index, Opcode::MOV, i.instruction_number, i.rarg);
    }
}

void NonAllocator::allocate() {
    destroy_phis();
    generate_data_section();
    std::cout << ir.to_dotlang();
    convert_ir();
}

void NonAllocator::convert_ir() {
    convert_main(ir.basic_blocks[0].successors.back());    
    for(const auto& block : ir.basic_blocks[0].successors | std::views::reverse | std::views::drop(1)) {
        convert_function(block);
    }
}

void NonAllocator::convert_main(const bb_t& b) {
    std::string atoi_str =
R"(
read:
    ; Syscall to read input
    mov eax, 3       ; sys_read
    mov ebx, 0       ; file descriptor 0 (stdin)
    mov ecx, buff    ; buffer to store input
    mov edx, 11      ; maximum number of bytes to read
    int 0x80         ; interrupt to call kernel

    ; Convert input string to integer
    mov esi, buff    ; ESI points to the input buffer
    xor eax, eax     ; Clear EAX (result)
    xor edi, edi     ; Clear EDI (multiplier)

convert_loop:
    movzx ecx, byte [esi]  ; Load current byte into ECX
    cmp ecx, 0x0A          ; Check for newline character
    je done                ; If newline, we're done
    sub ecx, '0'           ; Convert ASCII to integer
    imul eax, eax, 10      ; Multiply current result by 10
    add eax, ecx           ; Add the current digit to the result
    inc esi                ; Move to the next character
    jmp convert_loop       ; Repeat for next character

done:
    ret
)";
    std::string printeax_str = 
R"(
printeax:
    mov ecx, digitSpace
    mov ebx, 10
    mov [ecx], bl
    ; inc ecx
    mov [digitSpacePos], ecx

_printEAXLoop:
    xor edx, edx
    mov ebx, 10
    div ebx          ; Divide eax by 10, quotient in eax, remainder in edx
    push eax
    add dl, 48       ; Convert to ASCII

    mov ecx, [digitSpacePos]
    mov [ecx], dl
    inc ecx
    mov [digitSpacePos], ecx

    pop eax
    test eax, eax    ; Check if eax is 0
    jnz _printEAXLoop

_printEAXLoop2:
    mov ecx, [digitSpacePos]

    mov eax, 4       ; sys_write
    mov ebx, 1       ; file descriptor (stdout)
    mov esi, ecx     ; buffer
    mov edx, 1       ; count
    int 0x80

    mov ecx, [digitSpacePos]
    dec ecx
    mov [digitSpacePos], ecx

    cmp ecx, digitSpace
    jge _printEAXLoop2

    cld                   ; Clear direction flag (to increment destination pointer)
    mov ecx, 100          ; Set the count to 100 bytes
    mov edi, digitSpace   ; Set the destination pointer to digitSpace

    mov al, 0             ; Set the value to be stored (0 in this case)
    rep stosb             ; Store AL (0) in memory, incrementing destination pointer (EDI), repeat ECX times

    ret
)";
    ofile << "section .bss" << std::endl << "buff resb 11" << std::endl;
    ofile << "section .text" << std::endl;
    ofile << "global _start" << std::endl;
    ofile << atoi_str << std::endl; 
    ofile << printeax_str << std::endl;
    ofile << std::endl << "_start:" << std::endl;
    convert_scope(b);
    ofile << "mov eax, 1" << std::endl;
    ofile << "mov ebx, 0" << std::endl;
    ofile << "int 0x80" << std::endl;
}

void NonAllocator::convert_function(const bb_t& b) {
    if(ir.basic_blocks[b].instructions.size() != 0) {
        ofile << std::format("function{}:", ir.basic_blocks[b].instructions[0].instruction_number) << std::endl;
    }
    convert_scope(b);
    ofile << "ret" << std::endl;
}

void NonAllocator::convert_scope(const bb_t& b) {
    convert_block(b);
    const std::vector<bb_t>& successors = ir.basic_blocks[b].successors;
    if(successors.size() == 0) {
        return;
    } else if(successors.size() == 1) {
        convert_scope(successors[0]);
    } else if(ir.basic_blocks[successors[0]].type == Blocktype::IF_FALLTHROUGH || ir.basic_blocks[successors[0]].type == Blocktype::WHILE_FALLTHROUGH) {
        convert_scope(successors[0]);
        convert_scope(successors[1]);
    } else {
        convert_scope(successors[1]);
        convert_scope(successors[0]);
    }
}

void NonAllocator::convert_block(const bb_t& b) {
    BasicBlock& block = ir.basic_blocks.at(b);
    if(block.processed) return;
    if(block.predecessors.size() == 2 && (!ir.basic_blocks[block.predecessors[0]].processed || !ir.basic_blocks[block.predecessors[1]].processed)) return;
    if(block.instructions.size() != 0) {
        ofile << std::format("branch{}:", block.instructions[0].instruction_number) << std::endl;
        for(const auto& instruct : block.instructions) {
            convert_instruction(instruct);
        }
    }
    if(block.branch_instruction.opcode != Opcode::EMPTY) {
        convert_instruction(block.branch_instruction);
    }
    block.processed = true;
}

void NonAllocator::convert_instruction(const Instruction& i) {
    switch(i.opcode) {
        case(Opcode::ADD):
            convert_argument(i.larg, "eax");
            convert_argument(i.rarg, "ebx");
            emit_double_arg("add", i);
            break;
        case(Opcode::SUB):
            convert_argument(i.larg, "eax");
            convert_argument(i.rarg, "ebx");
            emit_double_arg("sub", i);
            break;
        case(Opcode::MUL):
            convert_argument(i.larg, "eax");
            convert_argument(i.rarg, "ebx");
            ofile << "mul ebx" << std::endl;
            ofile << std::format("mov [nonconst{}], eax", i.instruction_number);
            break;
        case(Opcode::DIV):
            convert_argument(i.larg, "eax");
            convert_argument(i.rarg, "ebx");
            ofile << "xor edx, edx" << std::endl;
            ofile << "div ebx" << std::endl;
            ofile << std::format("mov [nonconst{}], eax", i.instruction_number);
            break;
        case(Opcode::CMP):
            convert_argument(i.larg, "eax");
            convert_argument(i.rarg, "ebx");
            emit_double_arg("cmp", i);
            break;
        case(Opcode::PHI):
            break;
        case(Opcode::BRA):
            ofile << std::format("jmp branch{}", i.larg);
            break;
        case(Opcode::BNE):
            ofile << std::format("jne branch{}", i.rarg);
            break;
        case(Opcode::BEQ):
            ofile << std::format("je branch{}", i.rarg);
            break;
        case(Opcode::BLE):
            ofile << std::format("jle branch{}", i.rarg);
            break;
        case(Opcode::BLT):
            ofile << std::format("jl branch{}", i.rarg);
            break;
        case(Opcode::BGE):
            ofile << std::format("jge branch{}", i.rarg);
            break;
        case(Opcode::BGT):
            ofile << std::format("jg branch{}", i.rarg);
            break;
        case(Opcode::JSR):
            ofile << std::format("call function{}", i.larg) << std::endl;
            ofile << std::format("mov [nonconst{}], eax", i.instruction_number);
            break;
        case(Opcode::RET):
            if(const_instructions.find(i.larg) != const_instructions.end()) {
                ofile << std::format("mov eax, [const{}]", i.larg) << std::endl;
            } else {
                ofile << std::format("mov eax, [nonconst{}]", i.larg) << std::endl;
            }
            ofile << "ret";
            break;
        case(Opcode::MOV):
            if(const_instructions.find(i.rarg) != const_instructions.end()) {
                ofile << std::format("mov eax, [const{}]", i.rarg) << std::endl;
            } else {
                ofile << std::format("mov eax, [nonconst{}]", i.rarg) << std::endl;
            }

            if(const_instructions.find(i.larg) != const_instructions.end()) {
                ofile << std::format("mov [const{}], eax", i.larg) << std::endl;
            } else {
                ofile << std::format("mov [nonconst{}], eax", i.larg) << std::endl;
            }
            break;
        case(Opcode::GETPAR):
            ofile << "pop ebx" << std::endl;
            ofile << "pop eax" << std::endl;
            ofile << "push ebx" << std::endl;
            ofile << std::format("mov [nonconst{}], eax", i.instruction_number);
            break;
        case(Opcode::SETPAR):
            if(const_instructions.find(i.larg) != const_instructions.end()) {
                ofile << std::format("mov eax, [const{}]", i.larg) << std::endl;
            } else {
                ofile << std::format("mov eax, [nonconst{}]", i.larg) << std::endl;
            }
            ofile << "push eax" << std::endl;
            break;
        case(Opcode::READ):
            ofile << "call read" << std::endl;
            ofile << std::format("mov [nonconst{}], eax", i.instruction_number);
            break;
        case(Opcode::WRITE):
            if(const_instructions.find(i.larg) != const_instructions.end()) {
                ofile << std::format("mov eax, [const{}]", i.larg) << std::endl;
            } else {
                ofile << std::format("mov eax, [nonconst{}]", i.larg) << std::endl;
            }
            ofile << "call printeax" << std::endl;
            break;
        case(Opcode::WRITENL):
            ofile << "mov eax, 4" << std::endl;
            ofile << "mov ebx, 1" << std::endl;
            ofile << "mov ecx, newline" << std::endl;
            ofile << "mov edx, newline_len" << std::endl;
            ofile << "int 0x80";
            break;
        default:
            break;        
    }
    ofile << std::endl;
}

void NonAllocator::emit_double_arg(const std::string& cmd, const Instruction& i) {
    ofile << std::format("{} eax, ebx", cmd) << std::endl;
    ofile << std::format("mov [nonconst{}], eax", i.instruction_number);
}

void NonAllocator::convert_argument(const instruct_t& arg, const std::string& reg) {
    if(arg == -1) return;
    if(const_instructions.find(arg) != const_instructions.end()) {
        ofile << std::format("mov {}, [const{}]", reg, arg) << std::endl;
    } else {
        ofile << std::format("mov {}, [nonconst{}]", reg, arg) << std::endl;
    }
}
