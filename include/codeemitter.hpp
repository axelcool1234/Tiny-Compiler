#ifndef CODEEMITTER_HPP
#define CODEEMITTER_HPP
#include "intermediaterepresentation.hpp"
#include <fstream>

class CodeEmitter {
public:
    CodeEmitter(IntermediateRepresentation&& ir);
    IntermediateRepresentation ir;
    std::ofstream ofile;
    void emit_code();
    void debug() const;
private:
    /* Code Emitting */
    std::string emit_block(const bb_t& b);
    std::string emit_instruction(const Instruction& i);
    std::string emit_basic(const Instruction& i, const std::string& opcode);

    std::string emit_branch(const instruct_t& i, const std::string& opcode);
    std::string emit_write(const Instruction& instruction);
    std::string emit_read(const Instruction& instruction);
    std::string emit_mov(const Instruction& instruction);

    std::string reg_str(const instruct_t& instruct);
};

#endif // CODEEMITTER_HPP
