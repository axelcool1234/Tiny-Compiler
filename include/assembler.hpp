#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <elf.h>
#include <fstream>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>


void write_hello();

constexpr int INSTR_REX     = 0;
constexpr int INSTR_OP      = 1;
constexpr int INSTR_MODRM   = 2;
constexpr int INSTR_SIB     = 3;
constexpr int INSTR_DISP    = 4;
constexpr int INSTR_IMM     = 5;
struct IntelInstruction {
    std::array<bool, 6> used_fields{};
    struct { uint8_t b : 1, x : 1, r : 1, w : 1, b4 : 1, b3 : 1, b2 : 1, b1 : 1; } rex;
    uint8_t opcode;
    struct { uint8_t rm : 3, reg : 3, mod  : 2; } modrm;
    struct { uint8_t base : 3, index : 3, scale  : 2; } sib;
    uint64_t displacement;
    uint64_t immediate;
};



class Assembler
{
public:
    /*
     * Constructs an Assembler object given an input assembly file.  This
     * assembler is hard-coded to produce 64-bit binaries from x86 intel
     * assembly.
     */
    Assembler(std::string infilename);

    /*
     * First pass through the assembly, construct a symbol table for access on
     * the second pass.
     */
    void read_symbols();

    /*
     * Second pass through the assembly, construct the executable using
     * the symbol table.
     */
    void create_binary();


private:
    IntelInstruction assemble_instruction(std::istream_iterator<std::string>& is);

    static void setREXW(IntelInstruction& result);

    static IntelInstruction create_mov(std::istream_iterator<std::string>& is);
    static IntelInstruction create_xor(std::istream_iterator<std::string>& is);
    static IntelInstruction create_div(std::istream_iterator<std::string>& is);
    static IntelInstruction create_push(std::istream_iterator<std::string>& is);
    static IntelInstruction create_pop(std::istream_iterator<std::string>& is);
    static IntelInstruction create_inc(std::istream_iterator<std::string>& is);
    static IntelInstruction create_dec(std::istream_iterator<std::string>& is);
    static IntelInstruction create_test(std::istream_iterator<std::string>& is);
    static IntelInstruction create_jmp(std::istream_iterator<std::string>& is);
    static IntelInstruction create_jne(std::istream_iterator<std::string>& is);

    inline static const std::unordered_map<std::string, std::function<IntelInstruction(std::istream_iterator<std::string>&)>> instructions {
        {"mov",     create_mov},
        {"movabs",  create_mov},
        {"xor",     create_xor},
        {"div",     create_div},
        {"push",    create_push},
        {"pop",     create_pop},
        {"inc",     create_inc},
        {"dec",     create_dec},
        {"test",    create_test},
    };

    std::ifstream infile;
    std::unordered_map<std::string, size_t> sym_table;
};



static const std::unordered_set<std::string> directives {
    ".section",
    ".global",
};

#endif
