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
    IntermediateRepresentation() : basic_blocks({ BasicBlock{0} }), doms( {0} ) {}
    void compute_dominators();
    int intersect(int b1, int b2) const;
    int new_block(const int& p);
    int new_block(const int& p, Blocktype t);
    int new_block(const int& p1, const int& p2);
    void add_instruction(const int& b, Opcode op, int x1, int x2); 
    void add_instruction(const int& b, Opcode op, int x1); 
    void add_instruction(const int& b, Opcode op); 
    std::string to_dotlang() const;
};

#endif // INTERMEDIATEREPRESENTATION_HPP
