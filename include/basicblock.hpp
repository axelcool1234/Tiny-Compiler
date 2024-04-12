#ifndef BASICBLOCK_HPP
#define BASICBLOCK_HPP

#include "instruction.hpp"
#include <vector>

class BasicBlock {
private:
  std::vector<Instruction> instructions;
  std::vector<BasicBlock*> predecessors;
  std::vector<BasicBlock*> successors;
public:
};

#endif // BASICBLOCK_HPP
