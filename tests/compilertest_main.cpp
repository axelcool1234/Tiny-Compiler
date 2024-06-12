#include "codeemitter.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include "registerallocator.hpp"
#include <cstring>

#define USAGE_MSG " INFILE [-d] [-o OUTFILE]"\
                  "\n  -d          Debug information"\
                  "\n  -o          Output is written to OUTFILE if specified. If unspecified, output is written to INFILE with .s as the extension."\

int main(int argc, char *argv[])
{
    // Simple usage message
    if (argc < 2) {
        std::cerr << argv[0] << USAGE_MSG << std::endl;
        return 1;
    }

    std::string input_file = argv[1];    

    // Extract the base name of the input file (everything before the last dot)
    size_t last_dot_pos = input_file.find_last_of('.');
    std::string file_name = (last_dot_pos == std::string::npos) ? input_file + ".s" : input_file.substr(0, last_dot_pos) + ".s";

    // Go through flags
    bool debug = false;
    bool output_name = false;
    for(int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            debug = true;
        }
        if (strcmp(argv[i], "-o") == 0) {
            if(i + 1 == argc) {
                std::cerr << argv[0] << USAGE_MSG << std::endl;
                return 1;
            }
            file_name = argv[i + 1];
            break;
        }
    }  

    // Open the input file as an ifstream
    std::ifstream stream(input_file);
    if (!stream) {
        std::cerr << "Error: could not open input file " << input_file << std::endl;
        return 1;
    }

    /* Parse */
    Parser p{ stream };
    p.parse();
    if(debug) p.print();

    /* Allocate Registers */
    RegisterAllocator r{ p.release_ir() };
    r.allocate_registers();

    /* Emit Assembly */
    CodeEmitter c { r.release_ir(), file_name };
    c.emit_code();
    if(debug) c.debug();
    

    /* Generate ELF Binary */
    /* Assembler Test */
    Assembler a{ file_name };
    a.read_symbols();
    a.read_program();
    a.create_binary();

    return 0;
}
