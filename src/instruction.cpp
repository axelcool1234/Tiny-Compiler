#include "instruction.hpp"
#include <format>

std::string Instruction::to_dotlang() const {
    std::string msg = std::format("{}: {}", instruction_number, opcode_str_list[opcode]);
    if(opcode == CONST) { msg += std::format(" #{}", larg); return msg; }
    if(opcode == EMPTY) { return msg; }
    if(larg != -1) msg += std::format(" ({})", larg);
    if(rarg != -1) msg += std::format(" ({})", rarg);
    return msg;
}

Instruction::Instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2) 
    : instruction_number(num), opcode(op), larg(x1), rarg(x2) {};
