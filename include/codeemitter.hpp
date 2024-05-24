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
    std::string emit_scope(const bb_t& b);
    std::string emit_function(const bb_t& b);
    std::string emit_block(const bb_t& b);
    std::string emit_instruction(const Instruction& i);
    std::string emit_basic(const Instruction& i, const std::string& opcode);
    std::string emit_branch(const instruct_t& i, const std::string& opcode);
    std::string reg_str(const instruct_t& instruction);
};

#endif // CODEEMITTER_HPP
