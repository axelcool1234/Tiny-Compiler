#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <elf.h>
#include <fstream>
#include <iterator>
#include <unordered_map>

#include "intelinstruction.hpp"


void write_hello();


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

    IntelInstruction create_instruction(std::istream_iterator<std::string>& is);

    IntelInstruction create_mov(std::istream_iterator<std::string>& is);
    IntelInstruction create_lea(std::istream_iterator<std::string>& is);

    IntelInstruction create_2opinstr(std::istream_iterator<std::string>& is, uint8_t code1, uint8_t code2, uint8_t code3, uint8_t ext);
    IntelInstruction create_add(std::istream_iterator<std::string>& is);
    IntelInstruction create_sub(std::istream_iterator<std::string>& is);
    IntelInstruction create_cmp(std::istream_iterator<std::string>& is);
    IntelInstruction create_mul(std::istream_iterator<std::string>& is);
    IntelInstruction create_div(std::istream_iterator<std::string>& is);
    IntelInstruction create_xor(std::istream_iterator<std::string>& is);
    IntelInstruction create_inc(std::istream_iterator<std::string>& is);
    IntelInstruction create_dec(std::istream_iterator<std::string>& is);
    IntelInstruction create_idiv(std::istream_iterator<std::string>& is);
    IntelInstruction create_imul(std::istream_iterator<std::string>& is);

    IntelInstruction create_push(std::istream_iterator<std::string>& is);
    IntelInstruction create_pop(std::istream_iterator<std::string>& is);

    IntelInstruction create_jmpinstr(std::istream_iterator<std::string>& is, uint8_t code1, uint8_t code2);
    IntelInstruction create_test(std::istream_iterator<std::string>& is);
    IntelInstruction create_jmp(std::istream_iterator<std::string>& is);
    IntelInstruction create_je(std::istream_iterator<std::string>& is);
    IntelInstruction create_jne(std::istream_iterator<std::string>& is);
    IntelInstruction create_jge(std::istream_iterator<std::string>& is);
    IntelInstruction create_jg(std::istream_iterator<std::string>& is);
    IntelInstruction create_jle(std::istream_iterator<std::string>& is);
    IntelInstruction create_jl(std::istream_iterator<std::string>& is);
    
    IntelInstruction create_cqto(std::istream_iterator<std::string>& is);

    IntelInstruction create_syscall(std::istream_iterator<std::string>& is);
    IntelInstruction create_ret(std::istream_iterator<std::string>& is);

    std::ifstream infile;
    std::unordered_map<std::string, size_t> sym_table;
};



#endif
