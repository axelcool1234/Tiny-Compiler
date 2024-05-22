#ifndef REGISTERALLOC_HPP
#define REGISTERALLOC_HPP
#include "intermediaterepresentation.hpp"
#include <unordered_set>
#include <fstream>

enum class Register {
    EAX,
    EBX,
    ECX,
    EDX,
    ESI,
    EDI,
    EDP,
    ESP,
    R8D,
    R9D,
    R10D,
    R11D,
    R12D,
    R13D,
    R14D,
    R15D,

    REG_COUNT,
    UNASSIGNED
};

class RegisterAlloc {
public:
    IntermediateRepresentation ir;
    RegisterAlloc(IntermediateRepresentation&& ir);
    std::ofstream ofile; // for visual output
    std::unordered_map<instruct_t, std::unordered_set<instruct_t>> interference;
    std::unordered_set<bb_t> phi_blocks;

    std::vector<Register> reg_assignments{static_cast<size_t>(ir.instruction_count), Register::UNASSIGNED};

    void calculate_liveness();
    void process_block(std::unordered_set<instruct_t> live_set, BasicBlock& block);
    void process_block(std::unordered_set<instruct_t> live_set, const std::unordered_set<instruct_t>& combine, BasicBlock& block);
    void associate(const instruct_t& instruct, const std::unordered_set<instruct_t>& live_set);
    void color_ir();
    void color_instruction(const Instruction& instruct);
};

#endif
