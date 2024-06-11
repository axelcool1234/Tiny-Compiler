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
    bool getting_pars = false;
    bool main = false;
    /* Code Emitting */
    // Several //
    std::string block(const bb_t& b);
    std::string instruction(const Instruction& i);

    // Singular //
    std::string prologue();
    std::string write(const Instruction& instruction);
    std::string read(const Instruction& instruction);
    std::string branch(const instruct_t& i, const std::string& opcode);
    std::string scale(const Instruction& i, const std::string& operator_str);
    std::string additive(const Instruction& i, const std::string& operand);
    std::string cmp(const Instruction& i);

    /* Instructions */
    std::string mov_instruction(const instruct_t& from, const instruct_t& to);
    std::string additive_instruction(const instruct_t& left, const instruct_t& right, const std::string& operand);

    /* Helpers */
    std::string reg_str(const instruct_t& instruct);
    bool is_virtual_reg(const instruct_t& instruct);
    int virtual_reg_offset(const instruct_t& instruct);
};

#endif // CODEEMITTER_HPP
