#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <elf.h>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>


void write_hello();

struct IntelInstruction {
    struct { uint8_t hdr : 4, w : 1, r : 1, x : 1, b : 1; } REX;
    std::variant<uint8_t,uint16_t> opcode;
    struct { uint8_t mod : 2, reg : 3, rm  : 3; } modrm;
    struct { uint8_t scale : 2, index : 3, base  : 3; } sib;
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
    IntelInstruction assemble_instruction(const std::vector<std::string>& instr);

    std::ifstream infile;
    std::unordered_map<std::string, Elf64_Addr> sym_table;
    std::vector<uint8_t> bytecode;
};


// values are vector containing the opcode and the number of operands
static const std::unordered_map<std::string, std::vector<size_t>> instructions {
    {"syscall"  , {0x050f, 0}},
    {"ret"      , {0xc3, 0}},

    {"push"     , {0x50, 1}},
    {"pop"      , {0x58, 1}},
    {"inc"      , {0xff, 1}},
    {"dec"      , {0xff, 1}},
    {"je"       , {0x74, 1}},
    {"jne"      , {0x75, 1}},
    {"jmp"      , {0xe9, 1}},
    {"div"      , {0xf7, 1}},
    {"call"     , {0xe8, 1}},

    {"cmp"      , {0x00, 2}},

    {"mov"      , {0x00, 2}},
    {"movabs"   , {0x00, 2}},
    {"movzx"    , {0x00, 2}},

    {"lea"      , {0x00, 2}},
    {"xor"      , {0x00, 2}},
    {"add"      , {0x00, 2}},
    {"sub"      , {0x00, 2}},
    {"mul"      , {0x00, 2}},
    {"imul"     , {0x00, 2}},
    {"idiv"     , {0x00, 2}},
};

static const std::unordered_set<std::string> directives {
    ".section",
    ".global",
};

#endif
