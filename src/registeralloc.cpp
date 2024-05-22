#include "registeralloc.hpp"

RegisterAlloc::RegisterAlloc(IntermediateRepresentation&& ir) : ir(std::move(ir)), ofile("a.out") {}

void RegisterAlloc::calculate_liveness() {
    std::unordered_set<instruct_t> live_set;
    for(auto& b : ir.basic_blocks) {
        if(b.successors.size() == 0) process_block(live_set, b);
    }
}

void RegisterAlloc::process_block(std::unordered_set<instruct_t> live_set, BasicBlock& block) {
    for(const auto& s : block.successors) {
        if(!ir.basic_blocks[s].processed) return;
    }
    if(block.processed) return;

    std::unordered_set<instruct_t> left;    
    std::unordered_set<instruct_t> right;    
    for(const auto& instruct : block.instructions) {
        if(instruct.opcode == Opcode::BRA ||
           instruct.opcode == Opcode::BNE ||
           instruct.opcode == Opcode::BEQ ||
           instruct.opcode == Opcode::BLE ||
           instruct.opcode == Opcode::BLT ||
           instruct.opcode == Opcode::BGE ||
           instruct.opcode == Opcode::BGT) {
            continue;
        }

        if(live_set.find(instruct.instruction_number) != live_set.end()) {
            live_set.erase(instruct.instruction_number);            
            associate(instruct.instruction_number, live_set);
        }

        if(instruct.opcode == Opcode::PHI) {
            left.insert(instruct.larg);
            right.insert(instruct.rarg);
            phi_blocks.emplace(block.index);
            continue;
        }

        if(live_set.find(instruct.larg) == live_set.end()) {
            live_set.emplace(instruct.larg);
        }
        if(live_set.find(instruct.rarg) == live_set.end()) {
            live_set.emplace(instruct.rarg);
        }
    }
    block.processed = true;
    if(block.predecessors.size() >= 1) {
        process_block(live_set, left, ir.basic_blocks[0]);
    }
    if(block.predecessors.size() == 2) {
        process_block(live_set, right, ir.basic_blocks[1]);
    }
}

void RegisterAlloc::process_block(std::unordered_set<instruct_t> live_set, const std::unordered_set<instruct_t>& combine, BasicBlock& block) {
    live_set.insert(combine.begin(), combine.end());
    process_block(live_set, block);
}

void RegisterAlloc::associate(const instruct_t& instruct, const std::unordered_set<instruct_t>& live_set) {
    interference[instruct].insert(live_set.begin(), live_set.end());
    for(const auto& i : live_set) {
        interference[i].insert(instruct);
    }
}

void RegisterAlloc::color_ir() {
    for(const auto& b : phi_blocks) {
        for(const auto& instruct : ir.basic_blocks[b].instructions) {
            if(instruct.opcode != Opcode::PHI) break;
            color_instruction(instruct);
        }
    }
}

void RegisterAlloc::color_instruction(const Instruction& instruct) {
    if(reg_assignments[instruct.instruction_number] != Register::UNASSIGNED) return;


    bool i_l_intersect = interference[instruct.instruction_number].find(instruct.larg) != interference[instruct.instruction_number].end(); 
    bool i_r_intersect = interference[instruct.instruction_number].find(instruct.rarg) != interference[instruct.instruction_number].end(); 
    bool l_r_intersect = interference[instruct.larg].find(instruct.rarg) != interference[instruct.larg].end(); 

    std::vector<bool> available(static_cast<int>(Register::REG_COUNT), true);
    for(const auto& neighbor : interference[instruct.instruction_number]) {
        if(i_l_intersect) {
            if(i_r_intersect) {
                return;
            } else {
                try_assign(instruct.instruction_number, instruct.rarg);
            }
        } else if(i_r_intersect) {

        } else
        if (reg_assignments[instruct.larg] != Register::UNASSIGNED) {
            
        }

        if (reg_assignments[instruct.rarg] != Register::UNASSIGNED) {

        }
    }
}

void RegisterAlloc::try_assign(const instruct_t& i1, const instruct_t& i2);
