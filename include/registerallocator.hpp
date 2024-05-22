#ifndef REGISTERALLOCATOR_HPP
#define REGISTERALLOCATOR_HPP
#include "intermediaterepresentation.hpp"
#include <fstream>
#include <unordered_set>
/*
    // REGISTER(EAX, eax) \
    // REGISTER(EBX, ebx) \
    // REGISTER(ECX, ecx) \
    // REGISTER(EDX, edx) \
    // REGISTER(ESI, esi) \
    // REGISTER(EDI, edi) \
    // REGISTER(EDP, edp) \
    // REGISTER(ESP, esp) \
*/

#define REGISTER_LIST \
    REGISTER(R8D, r8d) \
    REGISTER(R9D, r9d) \
    REGISTER(R10D, r10d) \
    REGISTER(R11D, r11d) \
    REGISTER(R12D, r12d) \
    REGISTER(R13D, r13d) \
    REGISTER(R14D, r14d) \
    REGISTER(R15D, r15d)

enum Register {
    FIRST, 
#define REGISTER(name, str) name,
    REGISTER_LIST
#undef REGISTER
    LAST,
    UNASSIGNED
};

static const std::vector<std::string> reg_str_list {
#define REGISTER(name, str) #str,
    REGISTER_LIST
#undef REGISTER
};

class RegisterAllocator {
public:
    RegisterAllocator(IntermediateRepresentation&& ir);
    void allocate_registers();
    void emit_code();
    void debug() const;
private:
    /* Debug */
    void print_const_instructions() const;
    void print_live_ins() const;
    void print_leaf_blocks() const;
    void print_unanalyzed_blocks() const;
    void print_uncolored_blocks() const;
    void print_unemitted_blocks() const;
    void print_colored_instructions() const;
    void print_death_points() const;

    /* Code Emitting */
    std::string emit_scope(const bb_t& b);
    std::string emit_function(const bb_t& b);
    std::string emit_block(const bb_t& b);
    std::string emit_instruction(const Instruction& i);
    std::string emit_basic(const Instruction& i, const std::string& opcode);
    std::string emit_branch(const instruct_t& i, const std::string& opcode);
    std::string reg_str(const instruct_t& instruction);

    IntermediateRepresentation ir;
    std::ofstream ofile;
    std::vector<std::unordered_set<instruct_t>> live_ins {  ir.basic_blocks.size() };
    std::unordered_set<instruct_t> const_instructions {};
    std::unordered_map<instruct_t, int> const_instructions_map {};
    std::unordered_map<instruct_t, Register> assigned_registers;
    std::unordered_map<instruct_t, std::unordered_set<instruct_t>> death_points;

    void get_const_instructions();

    /* Liveness Analysis */
    void liveness_analysis();
    void analyze_block(BasicBlock& block);

    /* Color Graph */
    void color_ir();
    void color_block(BasicBlock& block);
    Register get_register(const Instruction& instruction, const std::unordered_set<Register>& occupied);
    void implement_phi_copies(BasicBlock& block, BasicBlock& phi_block);
};

#endif // REGISTERALLOCATOR_HPP
