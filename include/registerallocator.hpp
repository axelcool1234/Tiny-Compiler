#ifndef REGISTERALLOCATOR_HPP
#define REGISTERALLOCATOR_HPP
#include "intermediaterepresentation.hpp"
#include <unordered_set>

class RegisterAllocator {
public:
    RegisterAllocator(IntermediateRepresentation&& ir);
    void allocate_registers();
    IntermediateRepresentation release_ir();
    void debug() const;
private:
    IntermediateRepresentation ir;

    /* Liveness Analysis */
    void liveness_analysis();
    void analyze_block(const bb_t& block);
    void get_phi_liveness(const bb_t& block, const std::vector<Instruction>& instructions, const bool& left);
    bool non_reg_instruction(const Instruction& instruction);
    void check_argument_deaths(const Instruction& instruction, const bb_t& block);
    void apply_constraints(const Instruction& instruction, const bb_t& block);

    /* Color Graph */
    void color_ir();
    void color_block(const bb_t& block);
    Register get_register(const Instruction& instruction, const std::unordered_set<Register>& occupied);
    void implement_phi_copies(const bb_t& block, const bb_t& phi_block);
    void insert_phi_copies(const bb_t& block, const bb_t& phi_block, const bool& left);
};

#endif // REGISTERALLOCATOR_HPP
