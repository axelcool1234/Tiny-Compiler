#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <elf.h>
#include <fstream>
#include <unordered_map>


void write_hello();

enum GeneralRegister {
    RAX,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,

    R8 = 0,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
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
    std::ifstream infile;
    std::unordered_map<std::string, Elf64_Addr> sym_table;

    // Elf64_Ehdr elf_hdr;
    // Elf64_Phdr text_hdr;
    // Elf64_Phdr data_hdr;
};


// class AssemblyTokenizer
// {
// public:
//     size_t next_instruction();
//     size_t LC = 0;
// };



#endif
