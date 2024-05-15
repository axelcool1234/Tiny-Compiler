#include "intermediaterepresentation.hpp"
#include <algorithm>
#include <ranges>
#include <format>

IntermediateRepresentation::IntermediateRepresentation() : basic_blocks{0}, doms{0} {} 

// Cooper-Harvey-Kennedy 
// Iterative Reverse Postorder Dominance Algorithm
// This algorithm is implicitly ran just for new blocks that are added to the data structure.
// Only run this if the whole thing needs to be recalculated for some reason.
void IntermediateRepresentation::compute_dominators() {
    if(ignore) return;
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
    if(ignore) return -1;
    while(b1 != b2) {
        while(b1 > b2) b1 = doms[b1];
        while(b2 > b1) b2 = doms[b2];
    }
    return b1;
}

bb_t IntermediateRepresentation::new_function(const bb_t& idom, const ident_t& ident_count) {
    // TODO: OUTDATED!
    bb_t index = basic_blocks.size();
    basic_blocks.emplace_back(index, ident_count, idom);
    doms.push_back(idom);
    return index;
}

bb_t IntermediateRepresentation::new_block_helper(const bb_t& p1, const bb_t& p2, const bb_t& idom, Blocktype t) {
    if(ignore) return -1;
    bb_t index = basic_blocks.size();
    if(t != Blocktype::INVALID) { // New block has 1 parent, and either FALLTHROUGH or BRANCH Blocktype.
        basic_blocks.emplace_back(index, basic_blocks[p1].identifier_values, p1, t);
    }
    else if(p2 != -1) { // New block has 2 parents, and is guaranteed to have the JOIN Blocktype.
      basic_blocks.emplace_back(index, basic_blocks[p1].identifier_values,
                                basic_blocks[p2].identifier_values, p1, p2,
                                instruction_count);
    }
    else { // New block has 1 parent and no specified Blocktype, so it'll be assigned the NONE Blocktype.
        basic_blocks.emplace_back(index, basic_blocks[p1].identifier_values, p1);
    }

    // Either the new_block function is called with 2 parents and a specified immediate dominator,
    // or the new_block function is called with 1 parent (which will trivially be its immediate dominator).
    if(idom != -1) {
        doms.push_back(idom);
    }
    else {
        doms.push_back(p1);
    }
    return index;
} 

bb_t IntermediateRepresentation::new_block(const bb_t& p) {
    if(ignore) return -1;
    return new_block_helper(p, -1, -1, Blocktype::INVALID);
}

bb_t IntermediateRepresentation::new_block(const bb_t& p, Blocktype t) {
    if(ignore) return -1;
    return new_block_helper(p, -1, -1, t);
}

bb_t IntermediateRepresentation::new_block(const bb_t& p1, const bb_t& p2, const bb_t& idom) {
    if(ignore) return -1;
    return new_block_helper(p1, p2, idom, Blocktype::INVALID);
}

instruct_t IntermediateRepresentation::change_empty(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
    if(basic_blocks[b].instructions.size() != 0 && basic_blocks[b].instructions.front().opcode == Opcode::EMPTY) {
        Instruction& instruction = basic_blocks[b].instructions.front();
        instruction.opcode = op;
        instruction.larg = larg;
        instruction.rarg = rarg;
        return instruction.instruction_number;
    }
    return -1;
}

instruct_t IntermediateRepresentation::add_instruction_helper(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg, const bool& prepend) {
    if(ignore) return -1;
    // Common Subexpression Elimination
    instruct_t instruct = search_cse(b, op, larg, rarg);
    if(instruct != -1) return instruct;

    // If the given block is empty, replace the EMPTY instruction with the given opcoode, larg, and rarg.
    instruct = change_empty(b, op, larg, rarg);
    if(instruct != -1) return instruct;

    // Whether we want to append (to the end) or prepend (at the beginning) the new instruction.
    if(prepend) {
        basic_blocks[b].prepend_instruction(++instruction_count, op, larg, rarg);  
    } else {
        basic_blocks[b].add_instruction(++instruction_count, op, larg, rarg);  
    }
    return instruction_count;
}

instruct_t IntermediateRepresentation::prepend_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, larg, rarg, true);
}
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, larg, rarg, false);
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& larg)  {
    if(ignore) return -1;
    return add_instruction_helper(b, op, larg, -1, false);
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, -1, -1, false);
}

void IntermediateRepresentation::set_return(const bb_t& b) {
    if(ignore) return;
    basic_blocks[b].will_return = true;
}

bool IntermediateRepresentation::will_return(const bb_t& b) const {
    if(ignore) return false;
    return basic_blocks[b].will_return;
}

void IntermediateRepresentation::set_branch_cond(const bb_t& b, Opcode op, const instruct_t& larg) {
    if(ignore) return;
    Instruction& instruction = basic_blocks[b].branch_instruction;
    if(will_return(b)) return;
    if(instruction.instruction_number == -1) instruction.instruction_number = ++instruction_count;
    instruction.opcode = op;
    instruction.larg = larg;
    if(op == Opcode::RET) set_return(b);
}

void IntermediateRepresentation::set_branch_location(const bb_t& b, const instruct_t& rarg) {
    if(ignore) return;
    Instruction& instruction = basic_blocks[b].branch_instruction;
    instruction.rarg = rarg;
}

instruct_t IntermediateRepresentation::search_cse(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
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
    if(ignore) return;
    if(will_return(branch_back)) return;
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
    if(ignore) return;
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
    if(ignore) return;
    // changed_idents<0, 1, 2>:
    // 0: index of identifier to change
    // 1: new instruction number to change to
    // 2: old instruction number to change from
    for(const auto& triplet : changed_idents) {
        basic_blocks[b].identifier_values[std::get<0>(triplet)] = std::get<1>(triplet);
        for(auto& instruct : basic_blocks[b].instructions) {
            if(skip_phi && instruct.opcode == PHI) continue;
            if(instruct.larg == std::get<2>(triplet)) instruct.larg = std::get<1>(triplet); 
            if(instruct.rarg == std::get<2>(triplet)) instruct.rarg = std::get<1>(triplet);
        }
    }
}

instruct_t IntermediateRepresentation::first_instruction(const bb_t& b) {
    if(ignore) return -1;
    if(basic_blocks[b].instructions.size() == 0) add_instruction(b, Opcode::EMPTY);
    return basic_blocks[b].instructions.front().instruction_number;
}

instruct_t IntermediateRepresentation::get_ident_value(const bb_t& b, const ident_t& ident) {    
    if(ignore) return -1;
    return basic_blocks[b].get_ident_value(ident);
}

void IntermediateRepresentation::change_ident_value(const bb_t& b, const ident_t& ident, const instruct_t& instruct) {
    if(ignore) return;
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
