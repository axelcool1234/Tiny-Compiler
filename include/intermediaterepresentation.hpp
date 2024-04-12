#ifndef INTERMEDIATEREPRESENTATION_HPP
#define INTERMEDIATEREPRESENTATION_HPP

#include "basicblock.hpp"
#include <vector>

class IntermediateRepresentation {
private:
  // std::vector<BasicBlock> basic_blocks;
public:
  void generate_control_flow_graph() const;
};

#endif // INTERMEDIATEREPRESENTATION_HPP
