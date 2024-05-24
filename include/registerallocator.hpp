#ifndef REGISTERALLOCATOR_HPP
#define REGISTERALLOCATOR_HPP
#include "intermediaterepresentation.hpp"
#include <fstream>
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

    /* Color Graph */
    void color_ir();
    void color_block(const bb_t& block);
    Register get_register(const Instruction& instruction, const std::unordered_set<Register>& occupied);
    void implement_phi_copies(const bb_t& block, const bb_t& phi_block);
};

#endif // REGISTERALLOCATOR_HPP
