#include "basicblock.hpp"

void BasicBlock::add_instruction(const int& num, Opcode op, const int& x1, const int& x2) {
  int index = (op < CSE_COUNT) ? op : OTHER;
  instructions.emplace_back(num, op, x1, x2);
  if(op < CSE_COUNT) sorted_instructions[op].emplace_back(instructions.size() - 1);
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
