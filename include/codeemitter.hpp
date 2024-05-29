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

    /** 
    * Syntax
    * mov <reg1>, <reg2> - if reg1 == reg2 no empty string added
    * mov <con>, <reg>
    **/
    std::string mov_instruction(const instruct_t& from, const instruct_t& to);

    std::string push_instruction(const instruct_t& i);
    std::string pop_instruction(const instruct_t& i);

    std::string div_instruction(const instruct_t& i);


    
    /*
    * Syntax - store in right register
    * add <reg2>, <reg1>
    * add <con>, <reg1>
    */
    std::string add_instruction(const instruct_t& left, const instruct_t& right);
    std::string add_emitter(const Instruction& i);
    
    /*
    * Syntax - store in right register
    * sub <reg2>, <reg1>
    * sub <con>, <reg1>
    */
    std::string sub_instruction(const instruct_t& left, const instruct_t& right);
    std::string sub_emitter(const Instruction& i);

    std::string reg_str(const instruct_t& instruct);

    bool is_virtual_reg(const instruct_t& instruct);
    int virtual_reg_offset(const instruct_t& instruct);
};

#endif // CODEEMITTER_HPP
