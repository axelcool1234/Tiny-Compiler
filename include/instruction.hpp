#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <string>

class Instruction {
private:
public:
  virtual std::string to_assembly() const = 0;
};

// Subclasses based off certain patterns will arise from this class
// Like a class for op <reg1> <reg2>

#endif // INSTRUCTION_HPP
