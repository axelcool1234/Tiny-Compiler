#ifndef REGISTERALLOCATOR_HPP
#define REGISTERALLOCATOR_HPP
#include "intermediaterepresentation.hpp"
#include <unordered_set>
#include <fstream>

class RegisterAllocator {
public:
    IntermediateRepresentation ir;
    std::ofstream ofile;
    int label_counter = 0;
    RegisterAllocator(IntermediateRepresentation&& ir);
    ~RegisterAllocator() = default;

    virtual void allocate() = 0;
    void generate_data_section();

    std::unordered_set<instruct_t> const_instructions;
private:
};

class SimpleAllocator : RegisterAllocator {
public:
    SimpleAllocator(IntermediateRepresentation&& ir);
    void allocate() override;
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

#endif // REGISTERALLOCATOR_HPP