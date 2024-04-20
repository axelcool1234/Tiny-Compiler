#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <map>
#include <string>
#include <format>
#include <unordered_set>
#include <vector>

// OTHER is an index for non-CSE opcodes
// CSE_COUNT is the size of the 2D vector in BasicBlock for storing CSE 
// instructions (and storing non-CSE opcodes in the OTHER index)
#define OPCODE_LIST \
    OPCODE(ADD, add) \
    OPCODE(SUB, sub) \
    OPCODE(MUL, mul) \
    OPCODE(DIV, div) \
    OPCODE(CSE_COUNT, ) \
    OPCODE(CMP, cmp) \
    OPCODE(PHI, phi) \
    OPCODE(END, end) \
    OPCODE(BRA, bra) \
    OPCODE(BNE, bne) \
    OPCODE(BEQ, beq) \
    OPCODE(BLE, ble) \
    OPCODE(BLT, blt) \
    OPCODE(BGE, bge) \
    OPCODE(BGT, bgt) \
    OPCODE(JSR, jsr) \
    OPCODE(RET, ret) \
    OPCODE(CONST, const) \
    OPCODE(GETPAR1, getpar1) \
    OPCODE(GETPAR2, getpar2) \
    OPCODE(GETPAR3, getpar3) \
    OPCODE(SETPAR1, setpar1) \
    OPCODE(SETPAR2, setpar2) \
    OPCODE(SETPAR3, setpar3) \
    OPCODE(READ, read) \
    OPCODE(WRITE, write) \
    OPCODE(WRITENL, writenl) \
    OPCODE(EMPTY, \\<empty\\>)

enum Opcode {
#define OPCODE(name, str) name,
    OPCODE_LIST
#undef OPCODE
};

static const std::vector<std::string> opcode_str_list {
#define OPCODE(name, str) #str,
    OPCODE_LIST
#undef OPCODE
};

using instruct_t = ssize_t;
struct Instruction {
    instruct_t instruction_number;
    Opcode opcode;
    instruct_t larg;
    instruct_t rarg;
    Instruction(const instruct_t& num, Opcode op, const instruct_t& x1, const instruct_t& x2);
    std::string to_dotlang() const;
};

#endif // INSTRUCTION_HPP
