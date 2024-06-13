#include "basicblock.hpp"
#include <format>
#include <iostream>

void BasicBlock::prepend_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2) {
    instructions.emplace(instructions.begin(), num, op, x1, x2);
    if(op == EMPTY) empty_index = instructions.size() - 1; 
    if(op < CSE_COUNT) partitioned_instructions[op].emplace_back(instructions.size() - 1);
}

void BasicBlock::add_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2) {
    instructions.emplace_back(num, op, x1, x2);
    if(op == EMPTY) empty_index = instructions.size() - 1;
    if(op < CSE_COUNT) partitioned_instructions[op].emplace_back(instructions.size() - 1);
}

void BasicBlock::prepend_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2, const ident_t& x1_owner, const ident_t& x2_owner) {
    instructions.emplace(instructions.begin(), num, op, x1, x2, x1_owner, x2_owner);
    if(op == EMPTY) empty_index = instructions.size() - 1;
    if(op < CSE_COUNT) partitioned_instructions[op].emplace_back(instructions.size() - 1);
}

void BasicBlock::add_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2, const ident_t& x1_owner, const ident_t& x2_owner) {
    instructions.emplace_back(num, op, x1, x2, x1_owner, x2_owner);
    if(op == EMPTY) empty_index = instructions.size() - 1;
    if(op < CSE_COUNT) partitioned_instructions[op].emplace_back(instructions.size() - 1);
}

instruct_t BasicBlock::get_ident_value(const ident_t& ident) {
    if(ident >= static_cast<ident_t>(identifier_values.size())) return -1;
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
    if(branch_instruction.instruction_number != -1) {
        if(instructions.size() != 0) msg += "|";
        msg += branch_instruction.to_dotlang();
    }
    msg += "}\"];\n";
    return msg;
}

BasicBlock::BasicBlock(const bb_t& i)                                    
    : partitioned_instructions(CSE_COUNT, std::vector<int>{}), type(NONE), index(i) {
        if(i == 0) add_instruction(0, Opcode::CONST, 0, -1);
}

BasicBlock::BasicBlock(const bb_t& i, const ident_t& ident_count, const bb_t& p)               
    : partitioned_instructions(CSE_COUNT, std::vector<int>{}), type(NONE), index(i), predecessors({p}), identifier_values(ident_count) {}

BasicBlock::BasicBlock(const bb_t& i, const std::vector<instruct_t>& dom_ident_vals, const bb_t& p)              
    : partitioned_instructions(CSE_COUNT, std::vector<int>{}), type(NONE), index(i), predecessors({p}), identifier_values(dom_ident_vals) {}

BasicBlock::BasicBlock(const bb_t& i, const std::vector<instruct_t>& dom_ident_vals, const bb_t& p, Blocktype t) 
    : partitioned_instructions(CSE_COUNT, std::vector<int>{}), type(t),    index(i), predecessors({p}), identifier_values(dom_ident_vals) {}

BasicBlock::BasicBlock(const bb_t& i, const bb_t& p1, const bb_t& p2, Blocktype t)    
   : partitioned_instructions(CSE_COUNT, std::vector<int>{}), type(JOIN), index(i), predecessors({p1, p2}) {}
