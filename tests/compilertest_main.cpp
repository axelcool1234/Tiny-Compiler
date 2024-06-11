#include "parser.hpp"
#include "assembler.hpp"

int main(int argc, char *argv[])
{
    // Parser p;
    // p.parse(); 
    // p.print();
    // write_hello();

    Assembler a{"rsrc/cqto_test.s"};
    a.read_symbols();

    return 0;
}
