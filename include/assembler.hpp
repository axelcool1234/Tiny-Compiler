#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <elf.h>



void write_elf();
Elf32_Ehdr init_elf32hdr();

class Assembler
{
public:
    Assembler();

private:
    Elf64_Ehdr elf_hdr;
    Elf64_Phdr text_hdr;
    Elf64_Phdr data_hdr;

};


#endif
