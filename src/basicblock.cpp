#include "basicblock.hpp"

void BasicBlock::add_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2) {
    instructions.emplace_back(num, op, x1, x2);
    if(op < CSE_COUNT) partitioned_instructions[op].emplace_back(instructions.size() - 1);
}

instruct_t BasicBlock::get_ident_value(const ident_t& ident) {
    return identifier_values[ident];
}

void BasicBlock::change_instruction(const ident_t& ident, const instruct_t& instruct) {
    identifier_values[ident] = instruct;
}

std::string BasicBlock::to_dotlang() const {
    std::string msg = std::format("bb{} [shape=record, label=\"<b>", index);
    if(type == JOIN) msg += "join\\n";
    msg += std::format("BB{} | ", index) + "{";
    for(size_t i = 0; i < instructions.size(); ++i) {
        msg += instructions[i].to_dotlang(); 
        if(i != instructions.size() - 1) msg += "|";
    }
    msg += "}\"];\n";
    return msg;
}

BasicBlock::BasicBlock(const bb_t& i, const ident_t& ident_count)                                    
    : partitioned_instructions(CSE_COUNT, std::vector<instruct_t>{}), type(NONE), index(i), identifier_values(ident_count) {}

BasicBlock::BasicBlock(const bb_t& i, const std::vector<ident_t>& dom_ident_vals, const bb_t& p)              
    : partitioned_instructions(CSE_COUNT, std::vector<instruct_t>{}), type(NONE), index(i), predecessors({p}), identifier_values(dom_ident_vals) {}

BasicBlock::BasicBlock(const bb_t& i, const std::vector<ident_t>& dom_ident_vals, const bb_t& p, Blocktype t) 
    : partitioned_instructions(CSE_COUNT, std::vector<instruct_t>{}), type(t),    index(i), predecessors({p}), identifier_values(dom_ident_vals) {}

BasicBlock::BasicBlock(const bb_t& i, const std::vector<ident_t>& dom_ident_vals, const bb_t& p1, const bb_t& p2)    
    : partitioned_instructions(CSE_COUNT, std::vector<instruct_t>{}), type(JOIN), index(i), predecessors({p1, p2}), identifier_values(dom_ident_vals) {}

