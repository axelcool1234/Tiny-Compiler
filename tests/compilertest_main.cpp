#include "parser.hpp"
#include "registerallocator.hpp"

int main(int argc, char *argv[])
{
    Parser p;
    p.parse(); 
    p.print();
    SimpleAllocator r{ p.release_ir() };
    r.allocate();
    return 0;
}
