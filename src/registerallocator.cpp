#include "registerallocator.hpp"
#include "intermediaterepresentation.hpp"
#include <format>
#include <iostream>
#include <ranges>

RegisterAllocator::RegisterAllocator(IntermediateRepresentation&& ir) : ir(std::move(ir)), ofile("a.out") {}
SimpleAllocator::SimpleAllocator(IntermediateRepresentation&& ir) : RegisterAllocator(std::move(ir)) {}

void RegisterAllocator::generate_data_section() {
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

void SimpleAllocator::destroy_phis() {
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

void SimpleAllocator::destroy_while_phi(const BasicBlock& b, const Instruction& i) {
    const BasicBlock& pred = ir.basic_blocks[b.predecessors[0]];
    const BasicBlock& branch_block = ir.basic_blocks[b.branch_block];
    ir.add_instruction(pred.index, Opcode::MOV, i.instruction_number, i.larg);
    ir.add_instruction(branch_block.index, Opcode::MOV, i.instruction_number, i.rarg);
}

void SimpleAllocator::destroy_if_phi(const BasicBlock& b, const Instruction& i) {
    const BasicBlock& pred1 = ir.basic_blocks[b.predecessors[0]];
    const BasicBlock& pred2 = ir.basic_blocks[b.predecessors[1]];
    if (pred1.type == Blocktype::FALLTHROUGH) {
        ir.add_instruction(pred1.index, Opcode::MOV, i.instruction_number, i.larg);
        ir.add_instruction(pred2.index, Opcode::MOV, i.instruction_number, i.rarg);
    } else {
        ir.add_instruction(pred2.index, Opcode::MOV, i.instruction_number, i.larg);
        ir.add_instruction(pred1.index, Opcode::MOV, i.instruction_number, i.rarg);
    }
}

void SimpleAllocator::allocate() {
    generate_data_section();
    destroy_phis();
    std::cout << ir.to_dotlang();
    convert_ir();
}

void SimpleAllocator::convert_ir() {
    convert_main(ir.basic_blocks[0].successors.back());    
    for(const auto& block : ir.basic_blocks[0].successors | std::views::reverse | std::views::drop(1)) {
        convert_function(block);
    }
}

void SimpleAllocator::convert_main(const bb_t& b) {
    std::string atoi_str =
R"(
atoi:
    ; Input:
    ;   eax: Pointer to the null-terminated string
    ; Output:
    ;   eax: Integer value
    
    xor     ebx, ebx        ; Clear ebx to store the result
    mov     ecx, eax        ; Copy the pointer to ecx (use ecx for string traversal)
    
    ; Handle negative sign
    mov     al, byte [ecx]  ; Load the first character
    cmp     al, '-'         ; Check if it's a negative sign
    jne     .not_negative   ; Jump if it's not negative
    inc     ecx             ; Move to the next character
    jmp     .check_digit    ; Jump to check for digits
    
.not_negative:
    
.check_digit:
    mov     al, byte [ecx]  ; Load the next character
    cmp     al, 0           ; Check for null terminator
    je      .done           ; If end of string, we are done
    
    cmp     al, '0'         ; Check if the character is a digit
    jl      .invalid_char   ; If it's less than '0', it's not a valid digit
    cmp     al, '9'         ; Check if the character is within '0' to '9' range
    jg      .invalid_char   ; If it's greater than '9', it's not a valid digit
    
    ; Multiply current result by 10
    imul    ebx, ebx, 10
    
    ; Convert ASCII digit to integer and add to result
    sub     al, '0'         ; Convert ASCII digit to integer value
    add     ebx, eax
    
    inc     ecx             ; Move to the next character
    jmp     .check_digit    ; Continue checking digits

.invalid_char:
    ; Handle invalid characters here if needed
    ; For simplicity, let's just return 0 in case of an invalid character
    xor     eax, eax        ; Clear eax (return 0)
    ret

.done:
    ; Check if the number was negative
    mov     al, byte [eax]  ; Load the first character again
    cmp     al, '-'         ; Check if it was negative
    jne     .skip_neg_check ; If it was not negative, skip this part
    neg     ebx             ; Negate the result
.skip_neg_check:
    
    ; Return the result
    mov     eax, ebx        ; Move the result to eax
    ret
)";
    std::string printeax_str = 
R"(
printeax:
    mov ecx, digitSpace
    mov ebx, 10
    mov [ecx], bl
    inc ecx
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
    ofile << "section .bss" << std::endl << "buff resb 32" << std::endl;
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

void SimpleAllocator::convert_function(const bb_t& b) {
    if(ir.basic_blocks[b].instructions.size() != 0) {
        ofile << std::format("function{}:", ir.basic_blocks[b].instructions[0].instruction_number) << std::endl;
    }
    convert_scope(b);
}

void SimpleAllocator::convert_scope(const bb_t& b) {
    // TODO: Incorrect! I'm doing a Depth-First Search rather than a Breadth-First Search!
    convert_block(b);
    const std::vector<bb_t>& successors = ir.basic_blocks[b].successors;
    if(successors.size() == 0) {
        return;
    } else if(successors.size() == 1) {
        convert_scope(successors[0]);
    } else if(ir.basic_blocks[successors[0]].type == Blocktype::FALLTHROUGH) {
        convert_scope(successors[0]);
        convert_scope(successors[1]);
    } else {
        convert_scope(successors[1]);
        convert_scope(successors[0]);
    }
}

void SimpleAllocator::convert_block(const bb_t& b) {
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

void SimpleAllocator::convert_instruction(const Instruction& i) {
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
            ofile << std::format("mov [nonconst{}], eax", i.larg);
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
            ofile << "pop eax" << std::endl;
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
            ofile << "mov eax, 3" << std::endl;
            ofile << "mov ebx, 0" << std::endl;
            ofile << "mov ecx, buff" << std::endl;
            ofile << "mov edx, 4" << std::endl;
            ofile << "int 0x80" << std::endl;
            ofile << "mov eax, buff" << std::endl;
            ofile << "call atoi" << std::endl;
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

void SimpleAllocator::emit_double_arg(const std::string& cmd, const Instruction& i) {
    ofile << std::format("{} eax, ebx", cmd) << std::endl;
    ofile << std::format("mov [nonconst{}], eax", i.instruction_number);
}

void SimpleAllocator::convert_argument(const instruct_t& arg, const std::string& reg) {
    if(arg == -1) return;
    if(const_instructions.find(arg) != const_instructions.end()) {
        ofile << std::format("mov {}, [const{}]", reg, arg) << std::endl;
    } else {
        ofile << std::format("mov {}, [nonconst{}]", reg, arg) << std::endl;
    }
}
