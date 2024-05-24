#include "codeemitter.hpp"
#include "parser.hpp"
// #include "nonallocator.hpp"
#include "registerallocator.hpp"

int main(int argc, char *argv[])
{
    /* Parse */
    Parser p;
    p.parse(); 
    p.print();

    /* Allocate Registers */
    RegisterAllocator r{ p.release_ir() };
    r.allocate_registers();

    /* Emit Assembly */
    CodeEmitter c { r.release_ir() };;
    c.emit_code();
    c.debug();

    /* Generate ELF Binary */
    return 0;
}
