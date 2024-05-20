#include <cstdint>
#include <vector>
#include <fstream>

#include <elf.h>
#include <string.h>

constexpr size_t VADDR_START = 0x08048000;


void write_elf()
{
    size_t program_offset{sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)*2};
    size_t program_size{program_offset};

    Elf32_Ehdr elf_hdr;
    elf_hdr.e_ident[EI_MAG0] = 0x7f;
    elf_hdr.e_ident[EI_MAG1] = 'E';
    elf_hdr.e_ident[EI_MAG2] = 'L';
    elf_hdr.e_ident[EI_MAG3] = 'F';
    elf_hdr.e_ident[EI_CLASS] = ELFCLASS32;
    elf_hdr.e_ident[EI_DATA] = ELFDATA2LSB;
    elf_hdr.e_ident[EI_VERSION] = EV_CURRENT;
    elf_hdr.e_ident[EI_OSABI] = ELFOSABI_LINUX;
    elf_hdr.e_ident[EI_ABIVERSION] = 0;
    bzero(elf_hdr.e_ident + EI_PAD, EI_NIDENT - EI_PAD);

    elf_hdr.e_type = ET_EXEC;
    elf_hdr.e_machine = EM_386;
    elf_hdr.e_version = EV_CURRENT;
    elf_hdr.e_entry = VADDR_START + program_offset;
    elf_hdr.e_phoff = sizeof(Elf32_Ehdr);
    elf_hdr.e_shoff = 0;
    elf_hdr.e_flags = 0;
    elf_hdr.e_ehsize = sizeof(Elf32_Ehdr);
    elf_hdr.e_phentsize = sizeof(Elf32_Phdr);
    elf_hdr.e_phnum = 2; // may change later for stack
    elf_hdr.e_shentsize = sizeof(Elf32_Shdr);
    elf_hdr.e_shnum = 0;
    elf_hdr.e_shstrndx = 0;

    Elf32_Phdr text_hdr;
    text_hdr.p_type = PT_LOAD;
    text_hdr.p_offset = program_offset;
    text_hdr.p_vaddr = VADDR_START + program_offset;
    text_hdr.p_paddr = 0;
    text_hdr.p_filesz = program_size; // to be updated later
    text_hdr.p_memsz = program_size;
    text_hdr.p_flags = PF_X | PF_W | PF_R;
    text_hdr.p_align = 0x1000;

    Elf32_Phdr data_hdr;
    data_hdr.p_type = PT_LOAD;
    data_hdr.p_offset = program_offset;
    data_hdr.p_vaddr = VADDR_START + program_offset;
    data_hdr.p_paddr = 0;
    data_hdr.p_filesz = program_size; // to be updated later
    data_hdr.p_memsz = program_size;
    data_hdr.p_flags = PF_X | PF_W | PF_R;
    data_hdr.p_align = 0x1000;

    std::vector<uint8_t> text{
        0xb8, 0x04, 0x00, 0x00, 0x00, // mov $4, %eax
        0xbb, 0x01, 0x00, 0x00, 0x00, // mov $1, %ebx
        0xb9, 0x00, 0x80, 0x04, 0x08, // mov $0x08048000, %ecx (address of "Hello, world!\n")
        0xba, 0x0e, 0x00, 0x00, 0x00, // mov $14, %edx
        0xcd, 0x80,                   // int $0x80 (syscall)
        0xb8, 0x01, 0x00, 0x00, 0x00, // mov $1, %eax
        0xbb, 0x02, 0x00, 0x00, 0x00, // mov $0, %ebx
        0xcd, 0x80                    // int $0x80 (syscall)
    };

    const char* hello = "Hello, world!\n";
    std::vector<uint8_t> data {hello, hello + strlen(hello)+1};

    std::ofstream file("my.out", std::ios::binary);

    // write elf header
    file.write(reinterpret_cast<const char*>(&elf_hdr), sizeof(elf_hdr));

    // write program headers
    file.write(reinterpret_cast<const char*>(&text_hdr), sizeof(text_hdr));
    file.write(reinterpret_cast<const char*>(&data_hdr), sizeof(data_hdr));

    // // Pad to start of text segment (0x1000)
    // std::vector<uint8_t> padding(0x1000 - sizeof(elf_hdr) - 2 * sizeof(Elf32_Phdr), 0);
    // file.write(reinterpret_cast<const char*>(padding.data()), padding.size());

    // Write text segment
    file.write(reinterpret_cast<const char*>(text.data()), text.size());

    // // Pad to start of data segment (0x2000)
    // padding = std::vector<uint8_t>(0x1000 - text.size(), 0);
    // file.write(reinterpret_cast<const char*>(padding.data()), padding.size());

    // Write data segment
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}




