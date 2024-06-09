#include <cstdint>
#include <iterator>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cstring>

#include <iostream>

#include "assembler.hpp"


constexpr size_t VADDR_START = 0x08048000;
constexpr size_t NUM_SECTIONS = 2;


static const std::unordered_map<std::string, uint8_t> registers {
    {"rax", 0},
        {"rcx", 1},
        {"rdx", 2},
        {"rbx", 3},
        {"rsp", 4},
        {"rbp", 5},
        {"rsi", 6},
        {"rdi", 7},
        {"r8" , 0},
        {"r9" , 1},
        {"r10", 2},
        {"r11", 3},
        {"r12", 4},
        {"r13", 5},
        {"r14", 6},
        {"r15", 7},
};


// TODO
// back to two pass, change the unoredered map to functions to just a function
// that returns other functions. c++ pattern matching sucks whatever deal with
// it.
//
// first past will basically just work when we have eveyr create_instruction
// function implemented. boiler plate work for the most part. then the same
// create functions will work for the second pass and creating the byte code.
// scaffolding for the second pass is already in this function commented out.
void Assembler::read_symbols()
{
    // kept in memory for now, maybe write to disk later
    size_t curr_offset{};
    std::istream_iterator<std::string> is{infile}, end{};

    while (is != end) {
        if (directives.contains(*is)) {
            ++is; ++is;

        } else if (instructions.contains(*is)) { // parse instruction into vector to analyze
            IntelInstruction ii = instructions.at(*is)(is);
            if (ii.used_fields[INSTR_REX])  { ++curr_offset; }
            if (ii.used_fields[INSTR_OP])   { ++curr_offset; }
            if (ii.used_fields[INSTR_MODRM]){ ++curr_offset; }
            if (ii.used_fields[INSTR_SIB])  { ++curr_offset; }
            if (ii.used_fields[INSTR_DISP]) { curr_offset += 8; }
            if (ii.used_fields[INSTR_IMM])  { curr_offset += 8; }

            std::vector<uint8_t> bytecode;
            if (ii.used_fields[INSTR_REX])  { bytecode.push_back(*reinterpret_cast<const uint8_t*>(&ii.rex));   }
            if (ii.used_fields[INSTR_OP])   { bytecode.push_back(*reinterpret_cast<const uint8_t*>(&ii.opcode));}
            if (ii.used_fields[INSTR_MODRM]){ bytecode.push_back(*reinterpret_cast<const uint8_t*>(&ii.modrm)); }
            if (ii.used_fields[INSTR_SIB])  { bytecode.push_back(*reinterpret_cast<const uint8_t*>(&ii.sib));   }
            if (ii.used_fields[INSTR_DISP]) {
                const uint8_t *temp = reinterpret_cast<const uint8_t*>(&ii.displacement);
                for (int i = 0; i < 8; ++i)
                    bytecode.push_back(temp[i]);
            }
            if (ii.used_fields[INSTR_IMM]) {
                const uint8_t *temp = reinterpret_cast<const uint8_t*>(&ii.immediate);
                for (int i = 0; i < 8; ++i)
                    bytecode.push_back(temp[i]);
            }
            for (auto i: bytecode)
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned)i << " ";
            std::cout <<  std::endl;

        } else {
            // std::cout << "something else: " << *is << std::endl;
            std::cout << "label at: " << std::hex << curr_offset << std::endl;
            ++is;

        }
    }

}


void Assembler::setREXW(IntelInstruction& result) {
    result.rex.b1 = 0;
    result.rex.b2 = 1;
    result.rex.b3 = 0;
    result.rex.b4 = 0;
    result.rex.w = 1;
    result.rex.r = 0;
    result.rex.x = 0;
    result.rex.b = 0;
    result.used_fields[INSTR_REX] = true;
}


IntelInstruction Assembler::create_mov(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    IntelInstruction result;
    if (op1.starts_with('$')) { // imm => reg
        op1.erase(op1.begin());
        op1.pop_back();
        op2.erase(op2.begin());

        setREXW(result);

        result.opcode = 0xb8 + registers.at(op2);
        result.used_fields[INSTR_OP] = true;

        result.immediate = std::stoul(op1);
        result.used_fields[INSTR_IMM] = true;
    } else if (op1.starts_with('%') && op2.starts_with('%')) {
        op1.erase(op1.begin());
        op1.pop_back();
        op2.erase(op2.begin());

        setREXW(result);

        result.opcode = 0x89;
        result.used_fields[INSTR_OP] = true;

        result.modrm.mod = 0b11;
        result.modrm.reg = registers.at(op1);
        result.modrm.rm = registers.at(op2);
    }

    return result;
}


IntelInstruction Assembler::create_xor(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    op1.erase(op1.begin());
    op1.pop_back();
    op2.erase(op2.begin());

    IntelInstruction result;

    setREXW(result);

    result.opcode = 0x31;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = registers.at(op1);
    result.modrm.rm = registers.at(op2);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_div(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    setREXW(result);

    result.opcode = 0xf7;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = 0b110;
    result.modrm.rm = registers.at(op1);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_push(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    result.opcode = 0x50 + registers.at(op1);
    result.used_fields[INSTR_OP] = true;

    return result;
}


IntelInstruction Assembler::create_pop(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    result.opcode = 0x58 + registers.at(op1);
    result.used_fields[INSTR_OP] = true;

    return result;
}


IntelInstruction Assembler::create_inc(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    setREXW(result);

    result.opcode = 0xff;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = 0b000;
    result.modrm.rm = registers.at(op1);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_dec(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    setREXW(result);

    result.opcode = 0xff;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = 0b001;
    result.modrm.rm = registers.at(op1);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_test(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    op1.erase(op1.begin());
    op1.pop_back();
    op2.erase(op2.begin());

    IntelInstruction result;

    setREXW(result);

    result.opcode = 0x85;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = registers.at(op1);
    result.modrm.rm = registers.at(op2);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_jne(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    IntelInstruction result;

    result.opcode = 0x75;
    result.used_fields[INSTR_OP] = true;

    // if (sym_table.contains(op1)) {
    //     // magic
    // } else {
    //     *reinterpret_cast<uint8_t*>(&result.modrm) = 0;
    // }
    result.used_fields[INSTR_MODRM] = true;

    return result;

}


IntelInstruction Assembler::create_jmp(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    IntelInstruction result;

    result.opcode = 0xeb;
    result.used_fields[INSTR_OP] = true;

    // if (sym_table.contains(op1)) {
    //     // magic
    // } else {
    //     *reinterpret_cast<uint8_t*>(&result.modrm) = 0;
    // }
    result.used_fields[INSTR_MODRM] = true;

    return result;
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
