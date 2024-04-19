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

class BasicBlock {
private:
    std::vector<Instruction> instructions;
    std::vector<std::vector<int>> sorted_instructions;
public:
    Blocktype type;
    int index;
    std::vector<int> predecessors;
    BasicBlock(int i)                     : sorted_instructions(CSE_COUNT, std::vector<int>{}), type(NONE), index(i) {}
    BasicBlock(int i, int p)              : sorted_instructions(CSE_COUNT, std::vector<int>{}), type(NONE), index(i), predecessors({p}) {}
    BasicBlock(int i, int p, Blocktype t) : sorted_instructions(CSE_COUNT, std::vector<int>{}), type(t),    index(i), predecessors({p}) {}
    BasicBlock(int i, int p1, int p2)     : sorted_instructions(CSE_COUNT, std::vector<int>{}), type(JOIN), index(i), predecessors({p1, p2}) {}
    void add_instruction(const int& num, Opcode op, const int& x1, const int& x2);
    std::string to_dotlang() const;
};

#endif // BASICBLOCK_HPP
