#ifndef BASICBLOCK_HPP
#define BASICBLOCK_HPP

#include "instruction.hpp"
#include <vector>
#include <algorithm>

enum Blocktype {
    FALLTHROUGH,
    BRANCH,
    JOIN,
    NONE
};
using ident_t = size_t;
using bb_t = size_t;
struct BasicBlock {
    std::vector<Instruction> instructions;
    std::vector<std::vector<instruct_t>> partitioned_instructions;
    Blocktype type;
    bb_t index;
    std::vector<bb_t> predecessors;
    std::vector<ident_t> identifier_values;
    BasicBlock(const bb_t& i, const ident_t& ident_count);
    BasicBlock(const bb_t& i, const std::vector<ident_t>& dom_ident_vals, const bb_t& p);              
    BasicBlock(const bb_t& i, const std::vector<ident_t>& dom_ident_vals, const bb_t& p, Blocktype t); 
    BasicBlock(const bb_t& i, const std::vector<ident_t>& dom_ident_vals, const bb_t& p1, const bb_t& p2);    
    void add_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2);
    instruct_t get_ident_value(const ident_t& ident);
    void change_instruction(const ident_t& ident, const instruct_t& instruct);
    std::string to_dotlang() const;
};

#endif // BASICBLOCK_HPP
