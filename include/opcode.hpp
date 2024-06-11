#ifndef OPCODE_HPP
#define OPCODE_HPP

#include <vector>
#include <string>

// CSE_COUNT is the size of the 2D vector in BasicBlock for storing CSE 
// instructions (and storing non-CSE opcodes in the OTHER index)
#define OPCODE_LIST \
    OPCODE(ADD, add) \
    OPCODE(SUB, sub) \
    OPCODE(MUL, mul) \
    OPCODE(DIV, div) \
    OPCODE(CONST, const) \
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
    OPCODE(GETPAR, getpar) \
    OPCODE(GETPAR1, getpar1) \
    OPCODE(GETPAR2, getpar2) \
    OPCODE(GETPAR3, getpar3) \
    OPCODE(SETPAR, setpar) \
    OPCODE(SETPAR1, setpar1) \
    OPCODE(SETPAR2, setpar2) \
    OPCODE(SETPAR3, setpar3) \
    OPCODE(MOV, mov) \
    OPCODE(SWAP, swap) \
    OPCODE(READ, read) \
    OPCODE(WRITE, write) \
    OPCODE(WRITENL, writenl) \
    OPCODE(EMPTY, \\<empty\\>) \
    OPCODE(DELETED, \\<deleted\\>) \

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

#endif // OPCODE_HPP
