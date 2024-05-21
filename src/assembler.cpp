#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>

#include <elf.h>

constexpr size_t VADDR_START = 0x08048000;
constexpr size_t NUM_SECTIONS = 2;


Elf32_Ehdr init_elf32hdr() {
    Elf32_Ehdr h;
    std::memcpy(h.e_ident, "\x7f""ELF", 4);
    h.e_ident[EI_CLASS] = ELFCLASS32;
    h.e_ident[EI_DATA] = ELFDATA2LSB;
    h.e_ident[EI_VERSION] = EV_CURRENT;
    h.e_ident[EI_OSABI] = ELFOSABI_LINUX;
    h.e_ident[EI_ABIVERSION] = 0;
    std::memset(h.e_ident + EI_PAD, 0, EI_NIDENT - EI_PAD);

    h.e_type = ET_EXEC;
    h.e_machine = EM_386;
    h.e_version = EV_CURRENT;
    h.e_entry = VADDR_START + 0x1000;
    h.e_phoff = sizeof(Elf32_Ehdr);
    h.e_shoff = 0;
    h.e_flags = 0;
    h.e_ehsize = sizeof(Elf32_Ehdr);
    h.e_phentsize = sizeof(Elf32_Phdr);
    h.e_phnum = NUM_SECTIONS; // may change later for stack
    h.e_shentsize = sizeof(Elf32_Shdr);
    h.e_shnum = 0;
    h.e_shstrndx = 0;

    return h;
}


void write_elf()
{
    // size_t program_offset{sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)*NUM_SECTIONS};
    // size_t program_size{program_offset};

    Elf32_Ehdr elf_hdr = init_elf32hdr();

    Elf32_Phdr text_hdr;
    text_hdr.p_type = PT_LOAD;
    text_hdr.p_offset = 0x1000;
    text_hdr.p_vaddr = VADDR_START + 0x1000;
    text_hdr.p_paddr = 0;
    text_hdr.p_filesz = 0;  // to be updated later
    text_hdr.p_memsz = 0;   // to be updated later
    text_hdr.p_flags = PF_X | PF_R;
    text_hdr.p_align = 0x1000;

    // Data section metadata depends on the size of the text section
    Elf32_Phdr data_hdr;
    data_hdr.p_type = PT_LOAD;
    data_hdr.p_offset = 0;  // to be updated later
    data_hdr.p_vaddr = 0;   // to be updated later
    data_hdr.p_paddr = 0;
    data_hdr.p_filesz = 0;  // to be updated later
    data_hdr.p_memsz = 0;   // to be updated later
    data_hdr.p_flags = PF_W | PF_R;
    data_hdr.p_align = 0x1000;

    std::vector<uint8_t> text {
        0xb8, 0x04, 0x00, 0x00, 0x00, // mov $4, %eax
        0xbb, 0x01, 0x00, 0x00, 0x00, // mov $1, %ebx
        0xb9, 0x00, 0xa0, 0x04, 0x08, // mov $0x08048000, %ecx (address of "Hello, world!\n")
        0xba, 0x0e, 0x00, 0x00, 0x00, // mov $14, %edx
        0xcd, 0x80,                   // int $0x80 (syscall)
        0xb8, 0x01, 0x00, 0x00, 0x00, // mov $1, %eax
        0xbb, 0x2a, 0x00, 0x00, 0x00, // mov $0, %ebx
        0xcd, 0x80                    // int $0x80 (syscall)
    };
    text_hdr.p_filesz = text.size();
    text_hdr.p_memsz = text.size();
    std::vector<uint8_t> text_padding(0x1000 - sizeof(Elf32_Ehdr) - NUM_SECTIONS * sizeof(Elf32_Phdr), 0);


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




