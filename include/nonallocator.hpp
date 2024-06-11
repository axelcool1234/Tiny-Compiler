#ifndef NONALLOCATOR_HPP
#define NONALLOCATOR_HPP
#include "intermediaterepresentation.hpp"
#include <unordered_set>
#include <fstream>

class NonAllocator {
public:
    IntermediateRepresentation ir;
    std::ofstream ofile;
    int label_counter = 0;
    std::unordered_set<instruct_t> const_instructions;
    NonAllocator(IntermediateRepresentation&& ir);
    void generate_data_section();
    void allocate(); 
    void destroy_phis();
    void destroy_while_phi(const BasicBlock& b, const Instruction& i);
    void destroy_if_phi(const BasicBlock& b, const Instruction& i);
    void convert_ir();
    void convert_scope(const bb_t& b);
    void convert_main(const bb_t& b);
    void convert_function(const bb_t& b);
    void convert_block(const bb_t& b);
    void convert_instruction(const Instruction& i);
    void convert_argument(const instruct_t& arg, const std::string& reg);

    void emit_double_arg(const std::string& cmd, const Instruction& i);
private:
};

#endif // NONALLOCATOR_HPP
