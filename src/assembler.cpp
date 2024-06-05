#include <cstdint>
#include <iterator>
#include <vector>
#include <fstream>
#include <cstring>
#include <unordered_set>
#include <ranges>

#include <iostream>

#include "assembler.hpp"


constexpr size_t VADDR_START = 0x08048000;
constexpr size_t NUM_SECTIONS = 2;

// instruction -> bytecode size
static const std::unordered_set<std::string, size_t> instructions_list {
    {"leaq"     , 16},
    {"movq"     , 16},
    {"movb"     , 16},
    {"movzxb"   , 16},
    {"movabsq"  , 16},

    {"xorq"     , 16},

    {"addq"     , 16},
    {"subq"     , 16},
    {"divq"     , 16},
    {"imulq"    , 16},
    {"incq"     , 16},
    {"decq"     , 16},

    {"cmpq"     , 16},
    {"cmpb"     , 16},

    {"jmp"      , 16},
    {"jne"      , 16},
    {"jge"      , 16},
    {"je"       , 16},

    {"pushq"    , 16},
    {"push"     , 16},
    {"pop"      , 16},

    {"call"     , 16},
    {"ret"      , 16},
    {"rep"      , 16},
    {"cld"      , 16},
    {"testq"    , 16},
    {"syscall"  , 16},
};

static const std::unordered_set<std::string> directives {
    ".section",
    ".global",
};


void Assembler::read_symbols()
{
    // TODO kept in memory for now, maybe write to disk
    // std::istream_iterator<std::string> it{infile}, end{};
    Elf64_Addr curr_addr{};

    // Anytime a label is used, store it's address in the table.  If we're
    // looking at the data section then store all variable names.
    for (std::string s: std::views::istream<std::string>(infile)) {
        if (s == ".section .data") {
            // read_data_symbols();
        } else if (s.ends_with(':')) {
            sym_table[s] = curr_addr;
        }
    }

}


Assembler::Assembler(std::string infilename)
    :infile{infilename}
{
    // ignore leading whitespace in the file
    infile >> std::ws;
}



void write_hello()
{
    Elf64_Ehdr elf_hdr = {
        .e_ident = {
            '\x7f','E','L','F', // magic
            ELFCLASS64,         // architecture class
            ELFDATA2LSB,        // little-endian
            EV_CURRENT,         // current-version
            ELFOSABI_LINUX,     // OS/ABI
            0,                  // ABI version
            0,0,0,0,0,0,0,      // padding
        },
        .e_type         = ET_EXEC,
        .e_machine      = EM_X86_64,
        .e_version      = EV_CURRENT,
        .e_entry        = VADDR_START + 0x1000,
        .e_phoff        = sizeof(Elf64_Ehdr),
        .e_shoff        = 0,
        .e_flags        = 0,
        .e_ehsize       = sizeof(Elf64_Ehdr),
        .e_phentsize    = sizeof(Elf64_Phdr),
        .e_phnum        = NUM_SECTIONS, // may change later for stack
        .e_shentsize    = sizeof(Elf64_Shdr),
        .e_shnum        = 0,
        .e_shstrndx     = 0,
    };

    Elf64_Phdr text_hdr {
        .p_type     = PT_LOAD,
        .p_flags    = PF_X | PF_R,
        .p_offset   = 0x1000,
        .p_vaddr    = VADDR_START + 0x1000,
        .p_filesz   = 0,   // to be updated later
        .p_memsz    = 0,   // to be updated later
        .p_align    = 0x1000,
    };

    // Data section metadata depends on the size of the text section
    Elf64_Phdr data_hdr {
        .p_type     = PT_LOAD,
        .p_flags    = PF_W | PF_R,
        .p_offset   = 0,   // to be updated later
        .p_vaddr    = 0,   // to be updated later
        .p_filesz   = 0,   // to be updated later
        .p_memsz    = 0,   // to be updated later
        .p_align    = 0x1000,
    };

    std::vector<uint8_t> text {
        0xb8, 0x04, 0x00, 0x00, 0x00, // mov $4, %eax
        0xbb, 0x01, 0x00, 0x00, 0x00, // mov $1, %ebx
        0xb9, 0x00, 0xa0, 0x04, 0x08, // mov $0x0804a000, %ecx (address of "Hello, world!\n")
        0xba, 0x0e, 0x00, 0x00, 0x00, // mov $14, %edx
        0xcd, 0x80,                   // int $0x80 (syscall)
        0xb8, 0x01, 0x00, 0x00, 0x00, // mov $1, %eax
        0xbb, 0x2a, 0x00, 0x00, 0x00, // mov $0, %ebx
        0xcd, 0x80                    // int $0x80 (syscall)
    };
    text_hdr.p_filesz = text.size();
    text_hdr.p_memsz = text.size();
    std::vector<uint8_t> text_padding(0x1000 - sizeof(Elf64_Ehdr) - NUM_SECTIONS * sizeof(Elf64_Phdr), 0);

    const char* msg = "Hello World\n";
    size_t msg_len = std::strlen(msg);
    data_hdr.p_offset = text_hdr.p_offset + text_hdr.p_memsz;
    data_hdr.p_offset = data_hdr.p_offset + (0x1000 - data_hdr.p_offset % 0x1000);
    data_hdr.p_vaddr = VADDR_START + data_hdr.p_offset;
    data_hdr.p_filesz = msg_len;
    data_hdr.p_memsz = msg_len;
    std::vector<uint8_t> data_padding(0x1000 - text_hdr.p_memsz, 0);


    std::ofstream file("my.out", std::ios::binary);

    // write elf header
    file.write(reinterpret_cast<const char*>(&elf_hdr), sizeof(elf_hdr));

    // write program headers
    file.write(reinterpret_cast<const char*>(&text_hdr), sizeof(text_hdr));
    file.write(reinterpret_cast<const char*>(&data_hdr), sizeof(data_hdr));

    // write padding before text segment
    file.write(reinterpret_cast<const char*>(text_padding.data()), text_padding.size());
    // Write text segment
    file.write(reinterpret_cast<const char*>(text.data()), text.size());

    // write padding before data segment
    file.write(reinterpret_cast<const char*>(data_padding.data()), data_padding.size());
    // Write data segment
    file.write(reinterpret_cast<const char*>(msg), msg_len);
    // file.write(reinterpret_cast<const char*>(data.data()), data.size());
}
