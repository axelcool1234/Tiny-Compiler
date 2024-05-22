#include "parser.hpp"
#include "nonallocator.hpp"
#include "registerallocator.hpp"

int main(int argc, char *argv[])
{
    Parser p;
    p.parse(); 
    p.print();
    RegisterAllocator r{ p.release_ir() };
    r.allocate_registers();
    r.emit_code();
    r.debug();
    // NonAllocator r{ p.release_ir() };
    // r.allocate();
    return 0;
}
