#include "intermediaterepresentation.hpp"
#include <algorithm>
#include <ranges>
#include <format>
#include <iostream>
#include <stdexcept>

IntermediateRepresentation::IntermediateRepresentation() : basic_blocks{0}, doms{0} {} 

void IntermediateRepresentation::init_live_ins() {
    live_ins.assign(basic_blocks.size(), std::unordered_set<instruct_t>());
}

// Cooper-Harvey-Kennedy 
// Iterative Reverse Postorder Dominance Algorithm
// This algorithm is implicitly ran just for new blocks that are added to the data structure.
// Only run this if the whole thing needs to be recalculated for some reason.
void IntermediateRepresentation::compute_dominators() {
    if(ignore) return;
    std::ranges::fill(doms, -1);
    doms[0] = basic_blocks[0].index; // entry block
    bool changed = true;
    while (changed) {
        changed = false;
        // for all basic_blocks, b, in reverse postorder (except first basic block)
        for (const BasicBlock &b : basic_blocks | std::views::drop(1)) {
            int new_idom = b.predecessors.front();
            // if b has a second predecessor
            if (b.predecessors.size() == 2 && doms[b.predecessors.back()] != -1) {
                new_idom = intersect(b.predecessors.back(), new_idom);
            }
            if (doms[b.index] != new_idom) {
                doms[b.index] = new_idom;
                changed = true;
            }
        }
    }
}

bb_t IntermediateRepresentation::intersect(bb_t b1, bb_t b2) const {
    if(ignore) return -1;
    while(b1 != b2) {
        while(b1 > b2) b1 = doms[b1];
        while(b2 > b1) b2 = doms[b2];
    }
    return b1;
}

bb_t IntermediateRepresentation::new_function(const bb_t& idom, const ident_t& ident_count) {
    bb_t index = basic_blocks.size();
    basic_blocks.emplace_back(index, ident_count, idom);
    basic_blocks[0].successors.emplace_back(index);
    doms.push_back(idom);
    return index;
}

bb_t IntermediateRepresentation::new_block_helper(const bb_t& p1, const bb_t& p2, const bb_t& idom, Blocktype t) {
    if(ignore) return -1;
    bb_t index = basic_blocks.size();
    if(t != Blocktype::INVALID) { // New block has 1 parent, and either FALLTHROUGH or BRANCH Blocktype.
        basic_blocks.emplace_back(index, basic_blocks[p1].identifier_values, p1, t);
        basic_blocks[p1].successors.emplace_back(index);
    }
    else if(p2 != -1) { // New block has 2 parents, and is guaranteed to have the JOIN Blocktype.
        basic_blocks.emplace_back(index, p1, p2, Blocktype::JOIN);
        basic_blocks[p1].successors.emplace_back(index);
        basic_blocks[p2].successors.emplace_back(index);
        // Generate phi functions for conflicting identifier values.
        for(size_t i = 0; i < basic_blocks[p1].identifier_values.size(); ++i) {
            if(basic_blocks[p1].identifier_values[i] != basic_blocks[p2].identifier_values[i]) {
                add_instruction(index, Opcode::PHI, basic_blocks[p1].identifier_values[i], basic_blocks[p2].identifier_values[i]);
                basic_blocks[index].identifier_values.emplace_back(instruction_count);
            } 
            else {
                basic_blocks[index].identifier_values.emplace_back(basic_blocks[p1].identifier_values[i]);
            }
        }
    }
    else { // New block has 1 parent and no specified Blocktype, so it'll be assigned the NONE Blocktype.
        basic_blocks.emplace_back(index, basic_blocks[p1].identifier_values, p1);
        basic_blocks[p1].successors.emplace_back(index);
    }

    // Either the new_block function is called with 2 parents and a specified immediate dominator,
    // or the new_block function is called with 1 parent (which will trivially be its immediate dominator).
    if(idom != -1) {
        doms.push_back(idom);
    }
    else {
        doms.push_back(p1);
    }
    return index;
} 

bb_t IntermediateRepresentation::new_block(const bb_t& p) {
    if(ignore) return -1;
    return new_block_helper(p, -1, -1, Blocktype::INVALID);
}

bb_t IntermediateRepresentation::new_block(const bb_t& p, Blocktype t) {
    if(ignore) return -1;
    return new_block_helper(p, -1, -1, t);
}

bb_t IntermediateRepresentation::new_block(const bb_t& p1, const bb_t& p2, const bb_t& idom) {
    if(ignore) return -1;
    return new_block_helper(p1, p2, idom, Blocktype::INVALID);
}

instruct_t IntermediateRepresentation::change_empty(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
    if(basic_blocks[b].instructions.size() != 0 && basic_blocks[b].instructions.front().opcode == Opcode::EMPTY) {
        Instruction& instruction = basic_blocks[b].instructions.front();
        instruction.opcode = op;
        instruction.larg = larg;
        instruction.rarg = rarg;
        return instruction.instruction_number;
    }
    return -1;
}

instruct_t IntermediateRepresentation::change_empty(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg) {
    if(ignore) return -1;
    if(basic_blocks[b].instructions.size() != 0 && basic_blocks[b].instructions.front().opcode == Opcode::EMPTY) {
        Instruction& instruction = basic_blocks[b].instructions.front();
        instruction.opcode = op;
        instruction.larg = larg.first;
        instruction.rarg = rarg.first;
        instruction.larg_owner = larg.second;
        instruction.rarg_owner = rarg.second;
        return instruction.instruction_number;
    }
    return -1;
}

instruct_t IntermediateRepresentation::add_instruction_helper(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg, const bool& prepend) {
    if(ignore) return -1;
    // Common Subexpression Elimination
    instruct_t instruct = search_cse(b, op, larg.first, rarg.first);
    if(instruct != -1) return instruct;

    // If the given block is empty, replace the EMPTY instruction with the given opcoode, larg, and rarg.
    instruct = change_empty(b, op, larg, rarg);
    if(instruct != -1) return instruct;

    // Whether we want to append (to the end) or prepend (at the beginning) the new instruction.
    if(prepend) {
        basic_blocks[b].prepend_instruction(++instruction_count, op, larg.first, rarg.first, larg.second, rarg.second);  
    } else {
        basic_blocks[b].add_instruction(++instruction_count, op, larg.first, rarg.first, larg.second, rarg.second);  
    }
    if(op == Opcode::CONST) const_instructions[instruction_count] = larg.first;
    // Not only place phis can be made. Check generate_phis function too!
    if(op == Opcode::PHI) establish_affinity_group(instruction_count, larg.first, rarg.first);
    return instruction_count;
}

instruct_t IntermediateRepresentation::prepend_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, { larg, -1 }, { rarg, -1 }, true);
}
instruct_t IntermediateRepresentation::prepend_instruction(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, larg, rarg, true);
}
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, larg, rarg, false);
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, { larg, -1 }, { rarg, -1 }, false);
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const instruct_t& larg)  {
    if(ignore) return -1;
    return add_instruction_helper(b, op, { larg, -1 }, { -1, -1 }, false);
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg)  {
    if(ignore) return -1;
    return add_instruction_helper(b, op, larg, { -1, -1 }, false);
} 
instruct_t IntermediateRepresentation::add_instruction(const bb_t& b, Opcode op) {
    if(ignore) return -1;
    return add_instruction_helper(b, op, { -1, -1 }, { -1, -1 }, false);
}

void IntermediateRepresentation::set_return(const bb_t& b) {
    if(ignore) return;
    basic_blocks[b].will_return = true;
}

bool IntermediateRepresentation::will_return(const bb_t& b) const {
    if(ignore) return false;
    return basic_blocks[b].will_return;
}

void IntermediateRepresentation::set_branch_cond(const bb_t& b, Opcode op, const instruct_t& larg) {
    if(ignore) return;
    Instruction& instruction = basic_blocks[b].branch_instruction;
    if(will_return(b)) return;
    if(instruction.instruction_number == -1) instruction.instruction_number = ++instruction_count;
    instruction.opcode = op;
    instruction.larg = larg;
    if(op == Opcode::RET) set_return(b);
}

void IntermediateRepresentation::set_branch_cond(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg) {
    if(ignore) return;
    Instruction& instruction = basic_blocks[b].branch_instruction;
    if(will_return(b)) return;
    if(instruction.instruction_number == -1) instruction.instruction_number = ++instruction_count;
    instruction.opcode = op;
    instruction.larg = larg.first;
    instruction.larg_owner = larg.second;
    if(op == Opcode::RET) set_return(b);
}

void IntermediateRepresentation::set_branch_location(const bb_t& b, const instruct_t& rarg) {
    if(ignore) return;
    Instruction& instruction = basic_blocks[b].branch_instruction;
    instruction.rarg = rarg;
}

instruct_t IntermediateRepresentation::search_cse(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg) {
    if(ignore) return -1;
    if(op > CSE_COUNT) return -1;
    bb_t curr_block = b;
    while(true) {
        for(const instruct_t& instruct : basic_blocks[b].partitioned_instructions[op]) {
            if(op == basic_blocks[b].instructions[instruct].opcode &&
               larg == basic_blocks[b].instructions[instruct].larg &&
               rarg == basic_blocks[b].instructions[instruct].rarg)
            return basic_blocks[b].instructions[instruct].instruction_number;
        }
        if(curr_block == 0) return -1;
        curr_block = doms[curr_block];
    }
}

void IntermediateRepresentation::generate_phi(const bb_t& loop_header, const bb_t& branch_back) {
    if(ignore) return;
    if(will_return(branch_back)) return;
    basic_blocks[loop_header].branch_block = branch_back;
    basic_blocks[branch_back].loop_header = loop_header;
    std::vector<instruct_t>& loop_ident_vals = basic_blocks[loop_header].identifier_values;
    const std::vector<instruct_t>& branch_ident_vals = basic_blocks[branch_back].identifier_values;
    std::vector<std::tuple<int, instruct_t, instruct_t>> changed_idents;

    for(size_t i = 0; i < loop_ident_vals.size(); ++i) {
        if(loop_ident_vals[i] != branch_ident_vals[i]) {
            instruct_t old_ident_val = loop_ident_vals[i];
            prepend_instruction(loop_header, Opcode::PHI, { loop_ident_vals[i], i }, { branch_ident_vals[i], i });
            changed_idents.emplace_back(i, instruction_count, old_ident_val);
        } 
    }

    update_ident_vals(loop_header, changed_idents, true);
    update_ident_vals_loop(basic_blocks[loop_header].successors[0], changed_idents); 
}

void IntermediateRepresentation::update_ident_vals_loop(const bb_t& curr_block, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents) {
    if(ignore) return;
    update_ident_vals(curr_block, changed_idents, false);
    if(basic_blocks[curr_block].successors.size() == 0) return;
    update_ident_vals_loop(basic_blocks[curr_block].successors[0], changed_idents);
    if(basic_blocks[curr_block].successors.size() == 2) {
        update_ident_vals_loop(basic_blocks[curr_block].successors[1], changed_idents);
    }
}
void IntermediateRepresentation::update_ident_vals_until(bb_t curr_block, bb_t stop_block, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents) {
    if(ignore) return;
    while(curr_block != stop_block) {
        bb_t idom = doms[curr_block];
        update_ident_vals(curr_block, changed_idents, false);
        if(basic_blocks[curr_block].predecessors.size() == 2) {
            update_ident_vals_until(basic_blocks[curr_block].predecessors[0], idom, changed_idents);
            update_ident_vals_until(basic_blocks[curr_block].predecessors[1], idom, changed_idents);
        }
        curr_block = idom;
    }
}

void IntermediateRepresentation::update_ident_vals(const bb_t& b, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents, const bool& skip_phi) {
    if(ignore) return;
    // changed_idents<0, 1, 2>:
    // 0: index of identifier to change
    // 1: new instruction number to change to
    // 2: old instruction number to change from
    for(const auto& triplet : changed_idents) {
        basic_blocks[b].identifier_values[std::get<0>(triplet)] = std::get<1>(triplet);
        for(auto& instruct : basic_blocks[b].instructions) {
            if(skip_phi && instruct.opcode == PHI) continue;
            if(instruct.larg == std::get<2>(triplet) && instruct.larg_owner == std::get<0>(triplet)) instruct.larg = std::get<1>(triplet); 
            if(instruct.rarg == std::get<2>(triplet) && instruct.rarg_owner == std::get<0>(triplet)) instruct.rarg = std::get<1>(triplet);
        }
    }
}

instruct_t IntermediateRepresentation::first_instruction(const bb_t& b) {
    if(ignore) return -1;
    if(basic_blocks[b].instructions.size() == 0) add_instruction(b, Opcode::EMPTY);
    return basic_blocks[b].instructions.front().instruction_number;
}

instruct_t IntermediateRepresentation::get_ident_value(const bb_t& b, const ident_t& ident) {    
    if(ignore) return -1;
    return basic_blocks[b].get_ident_value(ident);
}

void IntermediateRepresentation::change_ident_value(const bb_t& b, const ident_t& ident, const instruct_t& instruct) {
    if(ignore) return;
    basic_blocks[b].change_instruction(ident, instruct);
}

void IntermediateRepresentation::insert_live_in(const bb_t& b, const instruct_t& instruct) {
    live_ins.at(b).insert(instruct);
}

void IntermediateRepresentation::establish_affinity_group(const instruct_t& i1, const instruct_t& i2, const instruct_t& i3) {
    if(preference_list.find(i1) == preference_list.end() && !is_const_instruction(i1)) preference_list[i1] = Preference(); 
    if(preference_list.find(i2) == preference_list.end() && !is_const_instruction(i2)) preference_list[i2] = Preference(); 
    if(preference_list.find(i3) == preference_list.end() && !is_const_instruction(i3)) preference_list[i3] = Preference(); 
    if(!is_const_instruction(i1) && !is_const_instruction(i2)) {
        preference_list[i1].affinities.insert(i2); 
        preference_list[i2].affinities.insert(i1);
    }
    if(!is_const_instruction(i1) && !is_const_instruction(i3)) {
        preference_list[i1].affinities.insert(i3);
        preference_list[i3].affinities.insert(i1);    
    }
    if(!is_const_instruction(i2) && !is_const_instruction(i3)) {
        preference_list[i2].affinities.insert(i3);
        preference_list[i3].affinities.insert(i2);
    }
}

const std::vector<std::pair<Register, int>>& IntermediateRepresentation::get_instruction_preference(const instruct_t& instruct) {
    return preference_list.at(instruct).preference;
}

Preference& IntermediateRepresentation::get_preference(const instruct_t& instruct) {
    if(preference_list.find(instruct) != preference_list.end()) {
        return preference_list.at(instruct);
    } else {
        preference_list[instruct] = Preference();
        return preference_list.at(instruct);
    }
}
void IntermediateRepresentation::erase_live_in(const bb_t& b, const instruct_t& instruct) {
    live_ins.at(b).erase(instruct);
}

void IntermediateRepresentation::insert_death_point(const instruct_t& instruct, const instruct_t& death_point) {
    death_points[instruct].insert(death_point);
}

bool IntermediateRepresentation::propagate_live_ins(const bb_t& b) {
    // Ensure everything (the block's successors) before the block has been analyzed first.
    for(const auto& successor : basic_blocks[b].successors) {
        if(!is_analyzed(successor)) return false;
    }

    // Grab live SSA instructions from successors.
    for(const auto& successor : basic_blocks[b].successors) {
        live_ins[b].insert(live_ins[successor].begin(), live_ins[successor].end());
    }
    return true;
}

const std::vector<BasicBlock>& IntermediateRepresentation::get_basic_blocks() const {
    return basic_blocks;
}

const bb_t& IntermediateRepresentation::get_idom(const bb_t& b) const {
    return doms.at(b);
}

const std::vector<instruct_t>& IntermediateRepresentation::get_predecessors(const bb_t& b) const {
    return basic_blocks.at(b).predecessors;
}

const std::vector<instruct_t>& IntermediateRepresentation::get_successors(const bb_t& b) const {
    return basic_blocks.at(b).successors;
}

const std::vector<Instruction>& IntermediateRepresentation::get_instructions(const bb_t& b) const {
    return basic_blocks.at(b).instructions;
}

const instruct_t& IntermediateRepresentation::get_loop_header(const bb_t& b) const {
    if(basic_blocks.at(b).loop_header == -1) throw std::runtime_error("Given block does not have a loop header!");
    return basic_blocks.at(b).loop_header;
}

const instruct_t& IntermediateRepresentation::get_branch_back(const bb_t& b) const {
    if(basic_blocks.at(b).branch_block == -1) throw std::runtime_error("Given block does not have a branch block!");
    return basic_blocks.at(b).branch_block;
}

const std::unordered_set<instruct_t>& IntermediateRepresentation::get_live_ins(const bb_t& b) const {
    return live_ins.at(b);
}

const Register& IntermediateRepresentation::get_assigned_register(const instruct_t& instruct) const {
    return assigned_registers.at(instruct);
}

const Blocktype& IntermediateRepresentation::get_type(const bb_t& b) const {
    return basic_blocks.at(b).type;
}

const Instruction& IntermediateRepresentation::get_branch_instruction(const bb_t& b) const {
    return basic_blocks.at(b).branch_instruction;
}

const int& IntermediateRepresentation::get_const_value(const instruct_t& instruct) const {
    return const_instructions.at(instruct);
}

bool IntermediateRepresentation::is_live_instruction(const bb_t& b, const instruct_t& instruct) const {
    return live_ins.at(b).find(instruct) != live_ins.at(b).end();
}
bool IntermediateRepresentation::is_const_instruction(const instruct_t& instruct) const {
    return const_instructions.find(instruct) != const_instructions.end() || instruct == 0;
}

bool IntermediateRepresentation::is_valid_instruction(const instruct_t& instruct) const {
    // If -1, the instruction doesn't exist.
    return instruct != -1;
}

bool IntermediateRepresentation::is_undefined_instruction(const instruct_t& instruct) const {
    // If 0, the instruction is an uninitialized const value (of 0) for a user identifier.
    if(instruct == 0) {
        std::cout << "Warning! Variable is used before it is defined; defaulting to 0." << std::endl; 
        return true;
    }
    return false;
}

void IntermediateRepresentation::set_assigned_register(const instruct_t& instruct, const Register& reg) {
    if(assigned_registers.find(instruct) != assigned_registers.end()) throw std::runtime_error("Instruction already has an assigned register!");
    assigned_registers[instruct] = reg;
}

void IntermediateRepresentation::set_colored(const bb_t& b) {
    basic_blocks.at(b).colored = true;
}

void IntermediateRepresentation::set_analyzed(const bb_t& b) {
    basic_blocks.at(b).analyzed = true;   
} 

void IntermediateRepresentation::set_propagated(const bb_t& b) {
    basic_blocks.at(b).propagated = true;   
} 

void IntermediateRepresentation::set_emitted(const bb_t& b) {
    basic_blocks.at(b).emitted = true;   
} 

bool IntermediateRepresentation::has_assigned_register(const instruct_t& instruct) const {
    return assigned_registers.find(instruct) != assigned_registers.end();
}

bool IntermediateRepresentation::has_preference(const instruct_t& instruct) const {
    return preference_list.find(instruct) != preference_list.end();
}

bool IntermediateRepresentation::has_death_point(const instruct_t& instruct, const instruct_t& death_point) const {
    if(death_points.find(instruct) == death_points.end()) return false;
    return death_points.at(instruct).find(death_point) != death_points.at(instruct).end();    
}

bool IntermediateRepresentation::has_branch_instruction(const bb_t& b) const {
    return basic_blocks.at(b).branch_instruction.opcode != Opcode::EMPTY;
}

bool IntermediateRepresentation::has_zero_successors(const bb_t& b) const {
    return basic_blocks.at(b).successors.size() == 0;
}

bool IntermediateRepresentation::has_one_successor(const bb_t& b) const {
    return basic_blocks.at(b).successors.size() == 1;
}

bool IntermediateRepresentation::has_two_successors(const bb_t& b) const {
    return basic_blocks.at(b).successors.size() == 2;
}

bool IntermediateRepresentation::has_zero_predecessors(const bb_t& b) const {
    return basic_blocks.at(b).predecessors.size() == 0;
}

bool IntermediateRepresentation::has_one_predecessor(const bb_t& b) const {
    return basic_blocks.at(b).predecessors.size() == 1;
}

bool IntermediateRepresentation::has_two_predecessors(const bb_t& b) const {
    return basic_blocks.at(b).predecessors.size() == 2;
}

bool IntermediateRepresentation::is_loop_branch_back_related(const bb_t& loop_header, const bb_t& branch_back) const {    
    return basic_blocks.at(loop_header).branch_block == basic_blocks.at(branch_back).index && 
           basic_blocks.at(branch_back).loop_header  == basic_blocks.at(loop_header).index;  
}

bool IntermediateRepresentation::is_loop_header(const bb_t& b) const {
    return basic_blocks.at(b).branch_block != -1;
}

bool IntermediateRepresentation::is_branch_back(const bb_t& b) const {
    return basic_blocks.at(b).loop_header != -1;
}

bool IntermediateRepresentation::is_colored(const bb_t& b) const {
    return basic_blocks.at(b).colored;
}

bool IntermediateRepresentation::is_analyzed(const bb_t& b) const {
    return basic_blocks.at(b).analyzed;
}

bool IntermediateRepresentation::is_propagated(const bb_t& b) const {
    return basic_blocks.at(b).propagated;
}

bool IntermediateRepresentation::is_emitted(const bb_t& b) const {
    return basic_blocks.at(b).emitted;
}

bool IntermediateRepresentation::is_const_block(const bb_t& b) const {
    return b == 0;
}

std::string IntermediateRepresentation::to_dotlang() const {
    std::string msg = "digraph G {\n";
    for(const BasicBlock& b : basic_blocks) 
        msg += b.to_dotlang();
    for(const BasicBlock& b : basic_blocks){
        for(const auto& p : b.predecessors) {
            msg += std::format("bb{}:s -> bb{}:n ", p, b.index);
            if(b.type == IF_BRANCH || b.type == WHILE_BRANCH) {
                msg += "[label=\"branch\"]";
            }
            else if(b.type == IF_FALLTHROUGH || b.type == WHILE_FALLTHROUGH) {
                msg += "[label=\"fall-through\"]";
            }
            else if(b.type == JOIN && basic_blocks[p].type == IF_FALLTHROUGH) {
                msg += "[label=\"then-branch\"]";
            }
            else if(b.type == JOIN && basic_blocks[p].type == WHILE_BRANCH) {
                msg += "[label=\"od-fall-through\"]";
            }
            else if(b.type == JOIN && basic_blocks[p].type == IF_BRANCH) {
                msg += "[label=\"else-fall-through\"]";
            }
            else if(b.type == JOIN && basic_blocks[p].type == WHILE_FALLTHROUGH) {
                msg += "[label=\"do-branch\"]";
            }
            msg += ";\n";
        }       
    }
    for(size_t i = 1; i < doms.size(); ++i) {
        msg += std::format("bb{}:b -> bb{}:b [color=blue, style=dotted, label=\"dom\"]\n", doms[i], i);
    }
    msg += "}\n";
    return msg;
}

/* Debugging */
void IntermediateRepresentation::debug() const {
    print_const_instructions();
    print_leaf_blocks();
    print_live_ins();
    print_colored_instructions();
    print_death_points();
    print_unanalyzed_blocks();
    print_uncolored_blocks();
    print_unemitted_blocks();
    print_affinity_groups();
    print_preferences();

    std::cout << "--- After Phi Propagation ---" << std::endl;
    std::cout << to_dotlang();
}

void IntermediateRepresentation::print_const_instructions() const {
    std::cout << "--- Constant Instructions ---" << std::endl;
    for(const auto& instruction : const_instructions) {
        std::cout << "Instruction: " << instruction.first << " Value: " << instruction.second << std::endl;
    }
}

void IntermediateRepresentation::print_live_ins() const {
    std::cout << "--- Live-Ins ---" << std::endl;
    for(size_t block_index = 0; block_index < basic_blocks.size(); ++block_index) {
        std::cout << std::format("Block {}: ", block_index);
        for(const auto& live : live_ins[block_index]) {
            std::cout << live << ", ";
        }
        std::cout << std::endl;
    }
}

void IntermediateRepresentation::print_leaf_blocks() const {
    std::cout << "--- Leaf Blocks ---" << std::endl;
    for(const auto& block : basic_blocks) {
        if(block.successors.size() == 0) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void IntermediateRepresentation::print_unanalyzed_blocks() const {
    std::cout << "--- Unanalyzed Blocks ---" << std::endl;
    for(const auto& block : basic_blocks) {
        if(!block.analyzed) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void IntermediateRepresentation::print_uncolored_blocks() const {
    std::cout << "--- Uncolored Blocks ---" << std::endl;
    for(const auto& block : basic_blocks) {
        if(!block.colored) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void IntermediateRepresentation::print_unemitted_blocks() const {
    std::cout << "--- Unemitted Blocks ---" << std::endl;
    for(const auto& block : basic_blocks) {
        if(!block.emitted) {
            std::cout << "Block: " << block.index << std::endl;
        }
    }
}

void IntermediateRepresentation::print_colored_instructions() const {
    std::cout << "--- Colored Instructions ---" << std::endl;
    for(const auto& colored_instruction : assigned_registers) {
        std::cout << "Instruction: " << colored_instruction.first << " Register: " << colored_instruction.second << std::endl;
    }
}

void IntermediateRepresentation::print_death_points() const {
    std::cout << "--- Death Points ---" << std::endl;
    for(const auto& instruction_death: death_points) {
        std::cout << "Instruction: " << instruction_death.first << " Death: ";
        for(const auto& death : instruction_death.second) {
            std::cout << death << ", ";
        }
        std::cout << std::endl;
    }
}

void IntermediateRepresentation::print_affinity_groups() const {
    std::cout << "--- Affinity Groups ---" << std::endl;
    for(const auto& pref : preference_list) {
        std::cout << "Instruction: " << pref.first << ", Affinities: ";
        for(const auto& affinity : pref.second.affinities) {
            std::cout << affinity << ", ";
        }
        std::cout << std::endl;
    }
}

void IntermediateRepresentation::print_preferences() const {
    std::cout << "--- Instruction Preferences ---" << std::endl;
    for(const auto& pref : preference_list) {
        std::cout << "Instruction: " << pref.first << ", Preferences: ";
        auto copy = pref.second.preference;
        sort(copy.begin(), copy.end(), Preference::sort_by_preference);
        std::cout << "[";
        for(const auto& pair : copy) {
            std::cout << "(" << reg_str_list.at(pair.first) << ", " << pair.second << "), ";
        }
        std::cout << "]" << std::endl;
    }
}


/* Preference Struct */
Preference::Preference() : 
    preference{ 
    #define REGISTER(name, str) { name, 0 },
        REGISTER_LIST
    #undef REGISTER
    }
{}

void IntermediateRepresentation::constrain(const instruct_t& instruct, const bb_t& b, const Register& reg, const bool& propagate) {
    Preference& pref = get_preference(instruct);
    for(auto& pair : pref.preference) {
        if(pair.first == reg) continue;
        --pair.second;
    }
    if(propagate) {
        for(const auto& affinity : pref.affinities) {
            constrain(affinity, b, reg, false);
        }
        for(const auto& conflict : live_ins.at(b)) {
            if(conflict == instruct) continue;
            dislike(conflict, b, reg, false);
        }
    }
}

void IntermediateRepresentation::prefer(const instruct_t& instruct, const Register& reg, const bool& propagate) {
    Preference& pref = get_preference(instruct);
    ++pref.preference.at(reg).second;
    if(propagate) {
        for(const auto& affinity : pref.affinities) {
            prefer(affinity, reg, false);
        }
    }
}

void IntermediateRepresentation::dislike(const instruct_t& instruct, const bb_t& b, const Register& reg, const bool& propagate) {
    Preference& pref = get_preference(instruct);
    --pref.preference.at(reg).second; 
    if(propagate) {
        for(const auto& affinity : pref.affinities) {
            dislike(affinity, b, reg, false);
        }
    }
}
