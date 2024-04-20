#ifndef INTERMEDIATEREPRESENTATION_HPP
#define INTERMEDIATEREPRESENTATION_HPP

#include "basicblock.hpp"
#include <vector>
#include <algorithm>
#include <ranges>
#include <iostream>

class IntermediateRepresentation {
private:
    std::vector<BasicBlock> basic_blocks;
    std::vector<int> doms;
    int instruction_count = 0;
public:
    void compute_dominators();
    bb_t intersect(bb_t b1, bb_t b2) const;
    void establish_const_block(const ident_t& ident_count);
    bb_t new_block(const bb_t& p);
    bb_t new_block(const bb_t& p, Blocktype t);
    bb_t new_block(const bb_t& p1, const bb_t& p2);
    instruct_t add_instruction(const bb_t& b, Opcode op, const instruct_t& x1, const instruct_t& x2); 
    instruct_t add_instruction(const bb_t& b, Opcode op, const instruct_t& x1); 
    instruct_t add_instruction(const bb_t& b, Opcode op); 
    instruct_t search_cse(const bb_t& b, Opcode op, const instruct_t& x1, const instruct_t& x2);
    instruct_t get_ident_value(const bb_t& b, const ident_t& ident);
    void change_ident_value(const bb_t& b, const ident_t& ident, const instruct_t& instruct);
    std::string to_dotlang() const;
};

#endif // INTERMEDIATEREPRESENTATION_HPP
