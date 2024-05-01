#include "intermediaterepresentation.hpp"
#include <algorithm>
#include <ranges>
#include <format>

// Cooper-Harvey-Kennedy 
// Iterative Reverse Postorder Dominance Algorithm
// This algorithm is implicitly ran just for new blocks that are added to the data structure.
// Only run this if the whole thing needs to be recalculated for some reason.
void IntermediateRepresentation::compute_dominators() {
    std::ranges::fill(doms, -1);
    doms[0] = basic_blocks[0].index; // entry block
    bool changed = true;
    while (changed) {
        changed = false;
        // for all basic_blocks, b, in reverse postorder (except first basic block)
        for (const BasicBlock &b : basic_blocks | std::views::drop(1)) {
            int new_idom = b.predecessors.front();
            // if b has a second predecessor
            if (b.predecessors.size() == 2 && doms[b.predecessors.back()] != -1) {
                new_idom = intersect(b.predecessors.back(), new_idom);
            }
            if (doms[b.index] != new_idom) {
                doms[b.index] = new_idom;
                changed = true;
            }
        }
    }
}

bb_t IntermediateRepresentation::intersect(bb_t b1, bb_t b2) const {
    while(b1 != b2) {
        while(b1 > b2) b1 = doms[b1];
        while(b2 > b1) b2 = doms[b2];
    }
    return b1;
}

void IntermediateRepresentation::establish_const_block(const ident_t& ident_count) {
    basic_blocks.emplace_back(0, ident_count);
    doms.push_back(0);
}

bb_t IntermediateRepresentation::new_block(const bb_t& p) {
    bb_t index = basic_blocks.size();
    basic_blocks.emplace_back(index, basic_blocks[p].identifier_values, p);
    doms.push_back(p);
    return index;
}

bb_t IntermediateRepresentation::new_block(const bb_t& p, Blocktype t) {
    bb_t index = basic_blocks.size();
    basic_blocks.emplace_back(index, basic_blocks[p].identifier_values, p, t);
    doms.push_back(p);
    return index;
}

bb_t IntermediateRepresentation::new_block(const bb_t& p1, const bb_t& p2, const bb_t& idom) {
    bb_t index = basic_blocks.size();
    basic_blocks.emplace_back(index, basic_blocks[p1].identifier_values, basic_blocks[p2].identifier_values, 
                              p1, p2, instruction_count);
    doms.push_back(idom);
    return index;
}

instruct_t IntermediateRepresentation::change_empty(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(basic_blocks[b].instructions.size() != 0 && basic_blocks[b].instructions.front().opcode == Opcode::EMPTY) {
        Instruction& instruction = basic_blocks[b].instructions.front();
        instruction.opcode = op;
        instruction.larg = larg;
        instruction.rarg = rarg;
        return instruction.instruction_number;
    }
    return -1;
}
instruct_t IntermediateRepresentation::prepend_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    instruct_t instruct = search_cse(b, op, larg, rarg);
    if(instruct != -1) return instruct;
    instruct = change_empty(b, op, larg, rarg);
    if(instruct != -1) return instruct;
    basic_blocks[b].prepend_instruction(++instruction_count, op, larg, rarg);  
    return instruction_count;
}
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    instruct_t instruct = search_cse(b, op, larg, rarg);
    if(instruct != -1) return instruct;
    instruct = change_empty(b, op, larg, rarg);
    if(instruct != -1) return instruct;
    basic_blocks[b].add_instruction(++instruction_count, op, larg, rarg);  
    return instruction_count;
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& larg)  {
    instruct_t instruct = search_cse(b, op, larg, -1);
    if(instruct != -1) return instruct;
    instruct = change_empty(b, op, larg, -1);
    if(instruct != -1) return instruct;
    basic_blocks[b].add_instruction(++instruction_count, op, larg, -1);  
    return instruction_count;
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op) {
    instruct_t instruct = search_cse(b, op, -1, -1);
    if(instruct != -1) return instruct;
    instruct = change_empty(b, op, -1, -1); 
    if(instruct != -1) return instruct;
    basic_blocks[b].add_instruction(++instruction_count, op, -1, -1);  
    return instruction_count;
}

instruct_t IntermediateRepresentation::first_instruction(const bb_t& b) {
    if(basic_blocks[b].instructions.size() == 0) add_instruction(b, Opcode::EMPTY);
    return basic_blocks[b].instructions.front().instruction_number;
}

void IntermediateRepresentation::set_branch_cond(const bb_t& b, Opcode op, const instruct_t& larg) {
    Instruction& instruction = basic_blocks[b].branch_instruction;
    if(instruction.instruction_number == -1) instruction.instruction_number = ++instruction_count;
    instruction.opcode = op;
    instruction.larg = larg;
}

void IntermediateRepresentation::set_branch_location(const bb_t& b, const instruct_t& rarg) {
    Instruction& instruction = basic_blocks[b].branch_instruction;
    instruction.rarg = rarg;
}

instruct_t IntermediateRepresentation::search_cse(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(op > CSE_COUNT) return -1;
    bb_t curr_block = b;
    while(true) {
        for(const instruct_t& instruct : basic_blocks[b].partitioned_instructions[op]) {
            if(op == basic_blocks[b].instructions[instruct].opcode &&
               larg == basic_blocks[b].instructions[instruct].larg &&
               rarg == basic_blocks[b].instructions[instruct].rarg)
            return basic_blocks[b].instructions[instruct].instruction_number;
        }
        if(curr_block == 0) return -1;
        curr_block = doms[curr_block];
    }
}

void IntermediateRepresentation::generate_phi(const bb_t& loop_header, const bb_t& branch_back) {
    std::vector<instruct_t>& loop_ident_vals = basic_blocks[loop_header].identifier_values;
    const std::vector<instruct_t>& branch_ident_vals = basic_blocks[branch_back].identifier_values;
    std::vector<std::tuple<int, instruct_t, instruct_t>> changed_idents;

    for(size_t i = 0; i < loop_ident_vals.size(); ++i) {
        if(loop_ident_vals[i] != branch_ident_vals[i]) {
            instruct_t old_ident_val = loop_ident_vals[i];
            prepend_instruction(loop_header, Opcode::PHI, loop_ident_vals[i], branch_ident_vals[i]);
            changed_idents.emplace_back(i, instruction_count, old_ident_val);
        } 
    }
    bb_t curr_block = branch_back;
    update_ident_vals_until(branch_back, loop_header, changed_idents);    
    update_ident_vals(loop_header, changed_idents, true);
}

void IntermediateRepresentation::update_ident_vals_until(bb_t curr_block, bb_t stop_block, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents) {
    while(curr_block != stop_block) {
        bb_t idom = doms[curr_block];
        update_ident_vals(curr_block, changed_idents, false);
        if(basic_blocks[curr_block].predecessors.size() == 2) {
            update_ident_vals_until(basic_blocks[curr_block].predecessors[0], idom, changed_idents);
            update_ident_vals_until(basic_blocks[curr_block].predecessors[1], idom, changed_idents);
        }
        curr_block = idom;
    }
}

void IntermediateRepresentation::update_ident_vals(const bb_t& b, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents, const bool& skip_phi) {
    for(const auto& pair : changed_idents) {
        basic_blocks[b].identifier_values[std::get<0>(pair)] = std::get<1>(pair);
        for(auto& instruct : basic_blocks[b].instructions) {
            if(skip_phi && instruct.opcode == PHI) continue;
            if(instruct.larg == std::get<2>(pair)) instruct.larg = std::get<1>(pair); 
            if(instruct.rarg == std::get<2>(pair)) instruct.rarg = std::get<1>(pair);
        }
    }
}

instruct_t IntermediateRepresentation::get_ident_value(const bb_t& b, const ident_t& ident) {    
    return basic_blocks[b].get_ident_value(ident);
}

void IntermediateRepresentation::change_ident_value(const bb_t& b, const ident_t& ident, const instruct_t& instruct) {
    basic_blocks[b].change_instruction(ident, instruct);
}

std::string IntermediateRepresentation::to_dotlang() const {
    std::string msg = "digraph G {\n";
    for(const BasicBlock& b : basic_blocks) 
        msg += b.to_dotlang();
    for(const BasicBlock& b : basic_blocks){
        for(const auto& p : b.predecessors) {
            msg += std::format("bb{}:s -> bb{}:n ", p, b.index);
            if(b.type == BRANCH) {
                msg += "[label=\"branch\"]";
            }
            else if(b.type == FALLTHROUGH) {
                msg += "[label=\"fall-through\"]";
            }
            else if(b.type == JOIN && basic_blocks[p].type == FALLTHROUGH) {
                msg += "[label=\"branch\"]";
            }
            else if(b.type == JOIN && basic_blocks[p].type == BRANCH) {
                msg += "[label=\"fall-through\"]";
            }
            msg += ";\n";
        }       
    }
    for(size_t i = 2; i < doms.size(); ++i) {
        msg += std::format("bb{}:b -> bb{}:b [color=blue, style=dotted, label=\"dom\"]\n", doms[i], i);
    }
    msg += "}\n";
    return msg;
}
