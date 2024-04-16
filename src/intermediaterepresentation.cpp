#include "intermediaterepresentation.hpp"

// Cooper-Harvey-Kennedy 
// Iterative Reverse Postorder Dominance Algorithm
// This algorithm is implicitly ran just for new blocks that are added to the data structure.
// Only run this if the whole thing needs to be recalculated for some reason.
void IntermediateRepresentation::compute_dominators() {
  std::ranges::fill(doms, -1);
  doms[0] = basic_blocks[0].index; // entry block
  bool changed = true;
  while(changed) {
    changed = false;
    // for all basic_blocks, b, in reverse postorder (except first basic block)
    for(const BasicBlock& b : basic_blocks | std::views::drop(1)) {
      int new_idom = b.predecessors.front();
      // if b has a second predecessor
      if(b.predecessors.size() == 2 && doms[b.predecessors.back()] != -1) {
        new_idom = intersect(b.predecessors.back(), new_idom);
      }
      if (doms[b.index] != new_idom) {
        doms[b.index] = new_idom;
        changed = true;
      }
    }
  }
}

int IntermediateRepresentation::intersect(int b1, int b2) const {
  while(b1 != b2) {
    while(b1 > b2) b1 = doms[b1];
    while(b2 > b1) b2 = doms[b2];
  }
  return b1;
}

int IntermediateRepresentation::new_block(const int& p) {
  int index = static_cast<int>(basic_blocks.size());
  basic_blocks.emplace_back(index, p);
  doms.push_back(p);
  return index;
}

int IntermediateRepresentation::new_block(const int& p, Blocktype t) {
  int index = static_cast<int>(basic_blocks.size());
  basic_blocks.emplace_back(index, p, t);
  doms.push_back(p);
  return index;
}

int IntermediateRepresentation::new_block(const int& p1, const int& p2) {
  int index = static_cast<int>(basic_blocks.size());
  basic_blocks.emplace_back(index, p1, p2);
  doms.push_back(intersect(p1, p2));
  return index;
}

void IntermediateRepresentation::add_instruction(const int& b, Opcode op, int x1, int x2) {
  basic_blocks[b].add_instruction(++instruction_count, op, x1, x2);  
} 
void IntermediateRepresentation::add_instruction(const int& b, Opcode op, int x1)  {
  basic_blocks[b].add_instruction(++instruction_count, op, x1, -1);  
} 
void IntermediateRepresentation::add_instruction(const int& b, Opcode op) {
  basic_blocks[b].add_instruction(++instruction_count, op, -1, -1);  
} 

std::string IntermediateRepresentation::to_dotlang() const {
  std::string msg = "digraph G {\n";
  for(const BasicBlock& b : basic_blocks) 
    msg += b.to_dotlang();
  for(const BasicBlock& b : basic_blocks){
    for(const int& p : b.predecessors) {
      msg += std::format("bb{}:s -> bb{}:n ", p, b.index);
      if(b.type == BRANCH) {
        msg += "[label=\"branch\"]";
      }
      else if(b.type == FALLTHROUGH) {
        msg += "[label=\"fall-through\"]";
      }
      else if(b.type == JOIN && basic_blocks[p].type == FALLTHROUGH) {
        msg += "[label=\"branch\"]";
      }
      else if(b.type == JOIN && basic_blocks[p].type == BRANCH) {
        msg += "[label=\"fall-through\"]";
      }
      msg += ";\n";
    }       
  }
  for(size_t i = 2; i < doms.size(); ++i) {
    msg += std::format("bb{}:b -> bb{}:b [color=blue, style=dotted, label=\"dom\"]\n", doms[i], i);
  }
  msg += "}\n";
  return msg;
}

int main() {
  // Dotlang Test
  IntermediateRepresentation ir;
  int zero{0};
  int one = ir.new_block(zero);
  int two = ir.new_block(one, FALLTHROUGH);
  int three = ir.new_block(one, BRANCH);
  int four = ir.new_block(two, three);

  ir.add_instruction(one, READ);
  ir.add_instruction(one, ADD, 1, 1);
  ir.add_instruction(zero, CONST, 0);
  ir.add_instruction(one, CMP, 1, 3);
  ir.add_instruction(one, BGE, 4, 10);
  ir.add_instruction(two, ADD, 2, 2);
  ir.add_instruction(four, PHI, 6, 2);
  ir.add_instruction(four, PHI, 6, 1);
  ir.add_instruction(two, BRA, 7);
  ir.add_instruction(three, EMPTY);
  ir.add_instruction(four, WRITE, 8);
  std::cout << ir.to_dotlang();

  // Dominance Test (old and is incompatible to current design of IntermediateRepresentation)
  // IntermediateRepresentation ir;
  // BasicBlock& zero  = ir.get_entry_block();
  // BasicBlock& one   = ir.new_block(zero);
  // BasicBlock& two   = ir.new_block(one);
  // BasicBlock& three = ir.new_block(two);
  // BasicBlock& four  = ir.new_block(two);
  // BasicBlock& five  = ir.new_block(three, four);
  // BasicBlock& six   = ir.new_block(one);
  // BasicBlock& seven = ir.new_block(five, six);
  // // ir.compute_dominators();
  // ir.print_dominance_info();
  // BasicBlock& eight = ir.new_block(seven);
  // BasicBlock& nine  = ir.new_block(seven);
  // BasicBlock& ten   = ir.new_block(eight, nine);
  // std::cout << "compute dominators again:" << std::endl;
  // // ir.compute_dominators();
  // ir.print_dominance_info();
  // BasicBlock& eleven    = ir.new_block(ten);
  // BasicBlock& twelve    = ir.new_block(eleven);
  // BasicBlock& thirteen  = ir.new_block(eleven);
  // BasicBlock& fourteen  = ir.new_block(twelve, thirteen);
  // BasicBlock& fifteen   = ir.new_block(ten);
  // BasicBlock& sixteen   = ir.new_block(fifteen);
  // BasicBlock& seventeen = ir.new_block(fifteen);
  // BasicBlock& eighteen  = ir.new_block(sixteen, seventeen);
  // BasicBlock& nineteen  = ir.new_block(fourteen, eighteen); 
  // std::cout << "compute dominators again:" << std::endl;
  // // ir.compute_dominators();
  // ir.print_dominance_info();
}

/*
Postorder:         right, left, root
Reverse Postorder: root, left, right
       0    
       |
       1
      / \
     2   6 
    / \  |
   3  4  |
   \ /   |
    5    |
     \   |
      \  |
       \ |
        7
       / \
      8  9
      \ /
      10
     /  \
    /    \
   11    15
  /  \  /  \
 12 13 16  17
  \ /   \ /
  14     18
   \    /
    \  /
     19
*/
