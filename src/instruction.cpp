#include "instruction.hpp"

std::string Instruction::to_dotlang() const {
  std::string msg = std::format("{}: {}", instruction_number, opcode_str_list[opcode]);
  if(opcode == CONST) { msg += std::format(" #{}", larg); return msg; }
  if(opcode == EMPTY) { return msg; }
  if(larg != -1) msg += std::format(" ({})", larg);
  if(rarg != -1) msg += std::format(" ({})", rarg);
  return msg;
}

