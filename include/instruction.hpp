#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "opcode.hpp"

using ident_t = ssize_t;
using instruct_t = ssize_t;
struct Instruction {
    instruct_t instruction_number;
    Opcode opcode;
    instruct_t larg;
    instruct_t rarg;
    ident_t larg_owner = -1;
    ident_t rarg_owner = -1;
    Instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2);
    Instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2, const ident_t& x1_owner, const ident_t& x2_owner); 
    std::string to_dotlang() const;
};

#endif // INSTRUCTION_HPP
