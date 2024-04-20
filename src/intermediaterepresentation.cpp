#include "intermediaterepresentation.hpp"

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

bb_t IntermediateRepresentation::new_block(const bb_t& p1, const bb_t& p2) {
    bb_t index = basic_blocks.size();
    bb_t idom = intersect(p1, p2);
    basic_blocks.emplace_back(index, basic_blocks[idom].identifier_values, p1, p2);
    doms.push_back(idom);
    return index;
}

instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& x1, const instruct_t& x2) {
    instruct_t instruct = search_cse(b, op, x1, x2);
    if(instruct != -1) return instruct;
    basic_blocks[b].add_instruction(++instruction_count, op, x1, x2);  
    return instruction_count;
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& x1)  {
    instruct_t instruct = search_cse(b, op, x1, -1);
    if(instruct != -1) return instruct;
    basic_blocks[b].add_instruction(++instruction_count, op, x1, -1);  
    return instruction_count;
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op) {
    instruct_t instruct = search_cse(b, op, -1, -1);
    if(instruct != -1) return instruct;
    basic_blocks[b].add_instruction(++instruction_count, op, -1, -1);  
    return instruction_count;
}

instruct_t IntermediateRepresentation::search_cse(const bb_t& b, Opcode op, const instruct_t& x1, const instruct_t& x2) {
    if(op > CSE_COUNT) return -1;
    bb_t curr_block = b;
    while(true) {
        for(const instruct_t& instruct : basic_blocks[b].partitioned_instructions[op]) {
            if(op == basic_blocks[b].instructions[instruct].opcode &&
               x1 == basic_blocks[b].instructions[instruct].larg   &&
               x2 == basic_blocks[b].instructions[instruct].rarg)
            return basic_blocks[b].instructions[instruct].instruction_number;
        }
        if(curr_block == 0) return -1;
        curr_block = doms[curr_block];
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
