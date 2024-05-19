#ifndef BASICBLOCK_HPP
#define BASICBLOCK_HPP

#include "instruction.hpp"
#include <vector>

enum Blocktype {
    FALLTHROUGH,
    BRANCH,
    JOIN,
    NONE,
    INVALID, // A block should NEVER be this type. This exists for IntermediateRepresentation functions that want to
            // communicate that a Blocktype shouldn't be specified when creating a new BasicBlock. 
};
using ident_t = ssize_t;
using bb_t = ssize_t;
struct BasicBlock {
    std::vector<Instruction> instructions;
    std::vector<std::vector<instruct_t>> partitioned_instructions;
    Blocktype type;
    bool will_return = false; // If the block is guaranteed to return, this'll be true.
    bb_t index;
    std::vector<bb_t> predecessors;
    std::vector<bb_t> successors;
    std::vector<instruct_t> identifier_values;
    Instruction branch_instruction{-1, EMPTY, -1, -1};
    bb_t branch_block = -1; // If not -1, this is a while loop header
    bool processed = false;
    BasicBlock(const bb_t& i);
    BasicBlock(const bb_t& i, const ident_t& ident_count, const bb_t& p); 
    BasicBlock(const bb_t& i, const std::vector<instruct_t>& dom_ident_vals, const bb_t& p);              
    BasicBlock(const bb_t& i, const std::vector<instruct_t>& dom_ident_vals, const bb_t& p, Blocktype t); 
    BasicBlock(const bb_t& i, const std::vector<instruct_t>& p1_ident_vals, 
                const std::vector<instruct_t>& p2_ident_vals, const bb_t& p1, const bb_t& p2, int& instruction_count);    
    void add_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2);
    void prepend_instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2);
    instruct_t get_ident_value(const ident_t& ident);
    void change_instruction(const ident_t& ident, const instruct_t& instruct);
    std::string to_dotlang() const;
};

#endif // BASICBLOCK_HPP
