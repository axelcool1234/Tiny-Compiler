#ifndef INTELINSTRUCTION
#define INTELINSTRUCTION

#include <stdint.h>
#include <array>
#include <string>

constexpr int INSTR_REX     = 0;
constexpr int INSTR_OP      = 1;
constexpr int INSTR_MODRM   = 2;
constexpr int INSTR_SIB     = 3;
constexpr int INSTR_DISP    = 4;
constexpr int INSTR_IMM     = 12;

enum OpType {
    IMM,
    REG,
    REGADDR,
    ADDR
};

struct IntelInstruction {
    std::array<bool, 20> used_fields{};
    struct { uint8_t b : 1, x : 1, r : 1, w : 1, b4 : 1, b3 : 1, b2 : 1, b1 : 1; } rex{};
    uint8_t opcode;
    struct { uint8_t rm : 3, reg : 3, mod  : 2; } modrm;
    struct { uint8_t base : 3, index : 3, scale  : 2; } sib;
    int64_t displacement;
    int64_t immediate;

    void setREXW();
    void setREXB();
    void setREXR();
    static OpType get_optype(std::string op);
};

#endif /* ifndef SYMBOL */
