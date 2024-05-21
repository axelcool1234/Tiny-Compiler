#ifndef REGISTERALLOC_HPP
#define REGISTERALLOC_HPP
#include "intermediaterepresentation.hpp"
#include <unordered_set>
#include <fstream>



class RegisterAlloc {
public:
    IntermediateRepresentation ir;
    RegisterAlloc(IntermediateRepresentation&& ir);
    std::ofstream ofile; // for visual output

};

#endif