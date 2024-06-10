#include "intelinstruction.hpp"


void IntelInstruction::setREXW() {
    rex.b1 = 0;
    rex.b2 = 1;
    rex.b3 = 0;
    rex.b4 = 0;
    rex.w = 1;
}


void IntelInstruction::setREXB() {
    rex.b1 = 0;
    rex.b2 = 1;
    rex.b3 = 0;
    rex.b4 = 0;
    rex.b = 1;
}


OpType IntelInstruction::get_optype(std::string op) {
    if (op.ends_with(',')) {
        op.pop_back();
    }

    if (op.starts_with('$')) {
        return OpType::IMM;
    } else if (op.starts_with('%')) {
        return OpType::REG;
    } else if (op.ends_with(')')) {
        return (op.contains('%')) ? OpType::REGADDR : OpType::ADDR;
    }
    return IMM;
}
