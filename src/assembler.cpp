#include <algorithm>
#include <cstdint>
#include <iterator>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <cstring>

#include <iostream>

#include "assembler.hpp"
#include "intelinstruction.hpp"


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
};

static const std::unordered_map<std::string, uint8_t> extended_registers {
    {"r8" , 0},
    {"r9" , 1},
    {"r10", 2},
    {"r11", 3},
    {"r12", 4},
    {"r13", 5},
    {"r14", 6},
    {"r15", 7},
};


static const std::unordered_set<std::string> directives {
    ".section",
    ".global",
};

void Assembler::read_symbols()
{
    // kept in memory for now, maybe write to disk later
    size_t curr_offset{};
    std::istream_iterator<std::string> is{infile}, end{};

    while (is != end) {
        if (directives.contains(*is)) {
            ++is; ++is;

        } else if (instruction_mapping.contains(*is)) {
            IntelInstruction ii = create_instruction(is);

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
            const uint8_t *temp = reinterpret_cast<const uint8_t*>(&ii.displacement);
                for (int i = 0; i < 8; ++i)
                    if (ii.used_fields[INSTR_DISP+i])
                        bytecode.push_back(temp[i]);

            temp = reinterpret_cast<const uint8_t*>(&ii.immediate);
            for (int i = 0; i < 8; ++i)
                if (ii.used_fields[INSTR_IMM+i])
                    bytecode.push_back(temp[i]);

            for (auto i: bytecode)
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned)i << " ";
            std::cout <<  std::endl;

        } else {
            std::cout << "label at: " << std::hex << curr_offset << std::endl;
            ++is;

        }
    }

}


IntelInstruction Assembler::create_instruction(std::istream_iterator<std::string>& is) {
    InstructionType type = instruction_mapping.at(*is);
    IntelInstruction result;

    switch (type) {
        case MOV:
            result = create_mov(is);
            break;
        case LEA:
            result = create_lea(is);
            break;

        case ADD:
            result = create_add(is);
            break;
        case SUB:
            result = create_sub(is);
            break;
        case XOR:
            result = create_xor(is);
            break;
        case INC:
            result = create_inc(is);
            break;
        case DEC:
            result = create_dec(is);
            break;
        case MUL:
            result = create_mul(is);
            break;
        case DIV:
            result = create_div(is);
            break;

        case PUSH:
            result = create_push(is);
            break;
        case POP:
            result = create_pop(is);
            break;
        case TEST:
            result = create_test(is);
            break;
        case JMP:
            result = create_jmp(is);
            break;
        case JNE:
            result = create_jne(is);
            break;
        case NEG:
            //TODO: Do this
            // result = create_neg(is);
            break;
        case CQTO:
            //TODO: Do this
            // result = create_cqto(is);
            break;
    }

    return result;
};


IntelInstruction Assembler::create_mov(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    IntelInstruction result;
    OpType t1 = IntelInstruction::get_optype(op1);
    OpType t2 = IntelInstruction::get_optype(op2);

    if (t1 == IMM && t2 == REG) {
        result.opcode = 0xb8;

        result.setREXW();
        op2.erase(op2.begin()); // remove the %
        if (extended_registers.contains(op2)) {
            result.setREXB();
            result.opcode += extended_registers.at(op2);
        } else {
            result.opcode += registers.at(op2);
        }

        op1.erase(op1.begin()); // remove the $
        result.immediate = std::stoll(op1);

        result.used_fields[INSTR_REX] = true;
        result.used_fields[INSTR_OP] = true;
        std::fill_n(result.used_fields.begin() + INSTR_IMM, 8, true);

    } else if (t1 == IMM && (t2 == ADDR || t2 == REGADDR)) {
        result.opcode = 0xc7;

        op1.erase(op1.begin());
        result.immediate = std::stol(op1);

        if (t2 == REGADDR) {
            if (std::isalpha(op2.front())) {
                result.displacement = (sym_table.contains(op2)) ? sym_table.at(op2) : 0;
            } else {
                result.displacement = std::stol(op2);
            }

            op2.erase(op2.begin(), std::find(op2.begin(), op2.end(), '%'));
            op2.erase(op2.begin());
            op2.pop_back();

        // DOUBLE CHECK
            result.modrm.mod = 0b10;
            result.modrm.reg = 0b000;
            result.modrm.rm = registers.at(op2);
        } else {
            op2.erase(op2.begin());
            op2.pop_back();
            result.displacement = (sym_table.contains(op2)) ? sym_table.at(op2) : 0;

        // DOUBLE CHECK
            result.modrm.mod = 0b00;
            result.modrm.reg = 0b000;
            result.modrm.rm = 0b100;

            result.sib.scale = 0b00;
            result.sib.index = 0b100;
            result.sib.base = 0b101;
            result.used_fields[INSTR_SIB] = true;
        }

        result.used_fields[INSTR_OP] = true;
        result.used_fields[INSTR_MODRM] = true;
        std::fill_n(result.used_fields.begin() + INSTR_DISP, 4, true);
        std::fill_n(result.used_fields.begin() + INSTR_IMM, 4, true);
    } else if (t1 == REG && t2 == REG) {
         op1.erase(op1.begin());
         op1.pop_back();
         op2.erase(op2.begin());

         result.setREXW();
         result.opcode = 0x89;

        // DOUBLE CHECK
         result.modrm.mod = 0b11;
         result.modrm.reg = registers.at(op1);
         result.modrm.rm = registers.at(op2);

         result.used_fields[INSTR_REX] = true;
         result.used_fields[INSTR_OP] = true;
         result.used_fields[INSTR_MODRM] = true;
    } else if (t1 == REG && (t2 == ADDR || t2 == REGADDR)) {
        result.opcode = 0x89;
        result.setREXW();

        if (t2 == REGADDR) {
            if (std::isalpha(op2.front())) {
                result.displacement = (sym_table.contains(op2)) ? sym_table.at(op2) : 0;
            } else {
                result.displacement = std::stol(op2);
            }

        // DOUBLE CHECK
            result.modrm.mod = 0b10;
            result.modrm.reg = 0b000;

            op2.erase(op2.begin(), std::find(op2.begin(), op2.end(), '%'));
            op2.erase(op2.begin());
            op2.pop_back();
            if (extended_registers.contains(op2)) {
                result.setREXB();
                result.modrm.rm = extended_registers.at(op2);
            } else {
                result.modrm.rm = registers.at(op2);
            }
        } else {
            op2.erase(op2.begin());
            op2.pop_back();
            result.displacement = (sym_table.contains(op2)) ? sym_table.at(op2) : 0;

        // DOUBLE CHECK
            result.modrm.mod = 0b00;
            result.modrm.reg = 0b000;
            result.modrm.rm = 0b100;

            result.sib.scale = 0b00;
            result.sib.index = 0b100;
            result.sib.base = 0b101;
            result.used_fields[INSTR_SIB] = true;
        }

        result.used_fields[INSTR_REX] = true;
        result.used_fields[INSTR_OP] = true;
        result.used_fields[INSTR_MODRM] = true;
        std::fill_n(result.used_fields.begin() + INSTR_DISP, 4, true);
    } else if (t1 == REGADDR) {
        result.opcode = 0x8b;
        result.setREXW();

        if (std::isalpha(op1.front())) {
            result.displacement = (sym_table.contains(op1)) ? sym_table.at(op1) : 0;
        } else {
            result.displacement = std::stol(op1);
        }

        // DOUBLE CHECK
        op1.erase(op1.begin(), std::find(op1.begin(), op1.end(), '%'));
        op1.erase(op1.begin());
        op1.pop_back();
        op1.pop_back();
        op2.erase(op2.begin());

        result.modrm.mod = 0b10;
        result.modrm.rm = 0b001;

        if (extended_registers.contains(op2)) {
            result.setREXB();
            result.modrm.reg = extended_registers.at(op2);
        } else {
            result.modrm.reg = registers.at(op2);
        }

        result.used_fields[INSTR_REX] = true;
        result.used_fields[INSTR_OP] = true;
        result.used_fields[INSTR_MODRM] = true;
        std::fill_n(result.used_fields.begin() + INSTR_DISP, 4, true);
    } else {
        result.opcode = 0x8b;
        result.setREXW();

        op1.erase(op1.begin());
        op1.pop_back();
        op1.pop_back();
        op2.erase(op2.begin());

        result.displacement = (sym_table.contains(op2)) ? sym_table.at(op2) : 0;

        result.modrm.mod = 0b10;
        result.modrm.rm = 0b100;
        if (extended_registers.contains(op2)) {
            result.setREXB();
            result.modrm.reg = extended_registers.at(op2);
        } else {
            result.modrm.reg = registers.at(op2);
        }

        result.sib.scale = 0b00;
        result.sib.index = 0b100;
        result.sib.base = 0b101;
        result.used_fields[INSTR_SIB] = true;
        
        result.used_fields[INSTR_REX] = true;
        result.used_fields[INSTR_OP] = true;
        result.used_fields[INSTR_MODRM] = true;
        std::fill_n(result.used_fields.begin() + INSTR_DISP, 4, true);
    }


    return result;
}


IntelInstruction Assembler::create_lea(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    IntelInstruction result;

    if (op1.ends_with(')') && op2.starts_with('%')) {

    } else {
        op1.pop_back();
        op2.erase(op2.begin());

        result.setREXW();

        result.opcode = 0x8d;
        result.used_fields[INSTR_OP] = true;

        result.modrm.mod = 0b00;
        result.modrm.reg = registers.at(op2);
        result.modrm.rm = 0b100;
        result.used_fields[INSTR_MODRM] = true;

        result.sib.scale = 0b00;
        result.sib.index = 0b100;
        result.sib.base  = 0b101;
        result.used_fields[INSTR_SIB] = true;

        result.immediate = (sym_table.contains(op1)) ? sym_table.at(op1) : 0;
        std::fill_n(result.used_fields.begin() + INSTR_IMM, 4, true);
    }

    return result;
};


IntelInstruction Assembler::create_add(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    IntelInstruction result;

    if (op1.starts_with('$')) {
        op1.erase(op1.begin());
        op1.pop_back();
        op2.erase(op2.begin());

        result.setREXW();

        result.opcode = 0x81 + 0x00 + registers.at(op2);
        result.used_fields[INSTR_OP] = true;

        result.immediate = std::stoul(op1);
        std::fill_n(result.used_fields.begin() + INSTR_IMM, 8, true);

    } else if (op1.starts_with('%') && op2.starts_with('%')) {
        op1.erase(op1.begin());
        op1.pop_back();
        op2.erase(op2.begin());

        result.setREXW();

        result.opcode = 0x01;
        result.used_fields[INSTR_OP] = true;

        result.modrm.mod = 0b11;
        result.modrm.reg = registers.at(op1);
        result.modrm.rm = registers.at(op2);
        result.used_fields[INSTR_MODRM] = true;
    }

    return result;
}


IntelInstruction Assembler::create_sub(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    IntelInstruction result;

    if (op1.starts_with('$')) {
        op1.erase(op1.begin());
        op1.pop_back();
        op2.erase(op2.begin());

        result.setREXW();

        result.opcode = 0x81 + 0x05 + registers.at(op2);
        result.used_fields[INSTR_OP] = true;

        result.immediate = std::stoul(op1);
        std::fill_n(result.used_fields.begin() + INSTR_IMM, 8, true);

    } else if (op1.starts_with('%') && op2.starts_with('%')) {
        op1.erase(op1.begin());
        op1.pop_back();
        op2.erase(op2.begin());

        result.setREXW();

        result.opcode = 0x29;
        result.used_fields[INSTR_OP] = true;

        result.modrm.mod = 0b11;
        result.modrm.reg = registers.at(op1);
        result.modrm.rm = registers.at(op2);
        result.used_fields[INSTR_MODRM] = true;
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

    result.setREXW();

    result.opcode = 0x31;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = registers.at(op1);
    result.modrm.rm = registers.at(op2);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_inc(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    result.setREXW();

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

    result.setREXW();

    result.opcode = 0xff;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = 0b001;
    result.modrm.rm = registers.at(op1);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_mul(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    result.setREXW();

    result.opcode = 0xf7;
    result.used_fields[INSTR_OP] = true;

    result.modrm.mod = 0b11;
    result.modrm.reg = 0b110;
    result.modrm.rm = registers.at(op1);
    result.used_fields[INSTR_MODRM] = true;

    return result;
}


IntelInstruction Assembler::create_div(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    op1.erase(op1.begin());

    IntelInstruction result;

    result.setREXW();

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


IntelInstruction Assembler::create_test(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    std::string op2{*(++is)};
    ++is;

    op1.erase(op1.begin());
    op1.pop_back();
    op2.erase(op2.begin());

    IntelInstruction result;

    result.setREXW();

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

    if (sym_table.contains(op1)) {
        // magic
    } else {
        *reinterpret_cast<uint8_t*>(&result.modrm) = 0;
    }
    result.used_fields[INSTR_MODRM] = true;

    return result;

}


IntelInstruction Assembler::create_jmp(std::istream_iterator<std::string>& is) {
    std::string op1{*(++is)};
    ++is;

    IntelInstruction result;

    result.opcode = 0xeb;
    result.used_fields[INSTR_OP] = true;

    if (sym_table.contains(op1)) {
        // magic
    } else {
        *reinterpret_cast<uint8_t*>(&result.modrm) = 0;
    }
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
