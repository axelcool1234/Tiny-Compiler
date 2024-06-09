#include "registerallocator.hpp"
#include <iostream>
#include <map>
#include <queue>
#include <ranges>
#include <unordered_set>
#include <stdexcept>
#include <algorithm>

RegisterAllocator::RegisterAllocator(IntermediateRepresentation&& ir) : ir(std::move(ir)) {}

IntermediateRepresentation RegisterAllocator::release_ir() {
    return ir;
}

void RegisterAllocator::allocate_registers() {
    ir.init_live_ins();
    liveness_analysis();
    color_ir();
}

void RegisterAllocator::debug() const {
    ir.debug();
}

/* Liveness Analysis */
void RegisterAllocator::liveness_analysis() {
    for(const auto& block : ir.get_basic_blocks()) {
        if(ir.has_zero_successors(block.index)) {
            analyze_block(block.index);
        }
    }
}

void RegisterAllocator::analyze_block(const bb_t& block) {
    // Ensure block hasn't been analyzed yet.
    // If the block in question is the const block, skip it.
    if(ir.is_analyzed(block) || ir.is_const_block(block)) return;

    // Ensure everything (the block's successors) before the block has been analyzed first.
    // If they have been, grab the live SSA instructions from those successors.
    if(!ir.propagate_live_ins(block)) return;

    // Calculate deaths of instructions in future shuffle code
    // This is arguably unsafe due to it being a consequence of my parser implementation.
    //
    // Because of how phi functions are created in the parser, right arguments always
    // belong to the non-branching block, left arguments belong to branching blocks.
    //
    // Left arguments also belong to blocks above the phi_block in the case the phi_block is a
    // loop header.
    //
    // Right arguments also belong to branch-back blocks from the phi_block in the case the
    // phi_block is a loop header.

    // If we've propagated the phis already:
    // if(block.propagated) return;

    // Get liveness from loop header (that you're re-entering as the branch_back block)
    if(ir.is_branch_back(block)) {
        get_phi_liveness(block, ir.get_instructions(ir.get_loop_header(block)), false);
    } 
    // Get liveness from loop header you're entering into for the first time
    else if(ir.has_one_successor(block) && ir.is_loop_header(ir.get_successors(block).at(0))) {
        get_phi_liveness(block, ir.get_instructions(ir.get_successors(block).at(0)), true);
    }
    // Get liveness from JOIN block (as block branching to it)
    else if(ir.has_one_successor(block) && ir.has_branch_instruction(block)) {
        get_phi_liveness(block, ir.get_instructions(ir.get_successors(block).at(0)), true);
    } 
    // Get liveness from JOIN block (as block falling through to it)
    else if(ir.has_one_successor(block)) {
        get_phi_liveness(block, ir.get_instructions(ir.get_successors(block).at(0)), false);
    } 
    
    // Determine points of death and liveness of SSA instructions at the beginning of the block.
    for(const auto& instruction : ir.get_instructions(block) | std::views::reverse) {
        if(instruction.opcode == Opcode::DELETED) continue; 
        // When something is born, everything beyond this point will not have it as a living SSA instruction. 
        // Also check to see if it was alive before this - if it wasn't, it is considered dead code.
        if(!ir.is_live_instruction(block, instruction.instruction_number)) {
            ir.insert_death_point(instruction.instruction_number, instruction.instruction_number);
        }
        ir.erase_live_in(block, instruction.instruction_number);

        // Ignore arguments of phi instructions since the move/swap calls will be in the predecessors.
        // Also ignore branch instructions as they don't require registers to work correctly in x86.
        if(instruction.opcode == Opcode::PHI || non_reg_instruction(instruction)) {
             continue;
        }

        // When something dies, everything beyond this point will have it as a living SSA instruction.
        // An instruction is a candidate for death if:
        // It is not invalid (ie doesn't exist or the special "0" instruction for undefined user identifiers)
        // It is not a constant
        // It is not alive at this point.
        check_argument_deaths(instruction, block);

        // Apply constraints
        apply_constraints(instruction, block);
    }

    // This block has now been analyzed.
    ir.set_analyzed(block);

    // Analyze predecessor blocks.
    for(const auto& predecessor : ir.get_predecessors(block)) {
        analyze_block(predecessor);
    }
}

void RegisterAllocator::get_phi_liveness(const bb_t& block, const std::vector<Instruction>& instructions, const bool& left) {
    for(const auto& instruction : instructions) {
        if(instruction.opcode == Opcode::DELETED) continue;
        if(instruction.opcode != Opcode::PHI) break;
        if(left) {
            if(ir.is_const_instruction(instruction.larg)) continue;
            ir.insert_live_in(block, instruction.larg);
        } else {
            if(ir.is_const_instruction(instruction.rarg)) continue;
            ir.insert_live_in(block, instruction.rarg);
        }
    }
}

bool RegisterAllocator::non_reg_instruction(const Instruction& instruction) {
    return instruction.opcode == Opcode::BRA ||
           instruction.opcode == Opcode::BNE ||
           instruction.opcode == Opcode::BEQ ||
           instruction.opcode == Opcode::BLE ||
           instruction.opcode == Opcode::BLT ||
           instruction.opcode == Opcode::BGE ||
           instruction.opcode == Opcode::BGT ||
           instruction.opcode == Opcode::JSR ||
           instruction.opcode == Opcode::EMPTY; 
}

void RegisterAllocator::check_argument_deaths(const Instruction& instruction, const bb_t& block) {
    if(ir.is_valid_instruction(instruction.larg) && !ir.is_const_instruction(instruction.larg) && !ir.is_live_instruction(block, instruction.larg)) {
        ir.insert_live_in(block, instruction.larg);
        ir.insert_death_point(instruction.larg, instruction.instruction_number);
    }
    if(ir.is_valid_instruction(instruction.rarg) && !ir.is_const_instruction(instruction.rarg) && !ir.is_live_instruction(block, instruction.rarg)) {
        ir.insert_live_in(block, instruction.rarg);
        ir.insert_death_point(instruction.rarg, instruction.instruction_number);
    }
}

void RegisterAllocator::apply_constraints(const Instruction& instruction, const bb_t& block) {
    switch(instruction.opcode) {
        case Opcode::WRITE:
            ir.constrain(instruction.larg, block, RAX, true);
            break;
        case Opcode::READ:
            ir.constrain(instruction.instruction_number, block, RAX, true);
            break;
        case Opcode::RET:
            ir.constrain(instruction.instruction_number, block, RAX, true);
            break;
        case Opcode::JSR:
            ir.constrain(instruction.instruction_number, block, RAX, true);
            break;
        default:
            break;
    }
}

/* Graph Coloring */
void RegisterAllocator::color_ir() {
    // No need to color the const block.
    ir.set_colored(0);

    // Color every function in the program (including main).
    for(const auto& start_block : ir.get_successors(0)) {
        color_block(start_block);
    }
}

void RegisterAllocator::color_block(const bb_t& block) {
    // Ensure immediate dominator has been colored first.
    if(!ir.is_colored(ir.get_idom(block))) return;

    // If block is already colored, skip.
    if(ir.is_colored(block)) return;

    // Start occupied register set as empty.
    std::unordered_set<Register> occupied{};

    // For SSA instructions that are live at the beginning of the block, make sure
    // their assigned registers are occupied.
    for(const instruct_t& live_instruct : ir.get_live_ins(block)) {
        occupied.insert(ir.get_assigned_register(live_instruct));
    }

    // For SSA phi instructions, get registers for them and ignore their arguments.
    // The arguments of the phi functions are ignored because they correspond to move 
    // instructions in the current block's predecessors and hence don't represent live 
    // instructions in the current block.
    for(const auto& instruction : ir.get_instructions(block)) {
        if(instruction.opcode == Opcode::DELETED) continue;
        if(instruction.opcode != Opcode::PHI) break;
        ir.set_assigned_register(instruction.instruction_number, get_register(instruction, occupied));
        ir.prefer(instruction.instruction_number, ir.get_assigned_register(instruction.instruction_number), true);
        occupied.insert(ir.get_assigned_register(instruction.instruction_number));
    }

    // Assign registers
    for(const auto& instruction : ir.get_instructions(block)) {
        if(instruction.opcode == Opcode::DELETED || instruction.opcode == Opcode::PHI || instruction.opcode == Opcode::EMPTY) continue;

        // Unoccupy registers from dead SSA instructions
        if(ir.is_valid_instruction(instruction.larg) && ir.has_death_point(instruction.larg, instruction.instruction_number)) {
            occupied.erase(ir.get_assigned_register(instruction.larg));
        }
        if(ir.is_valid_instruction(instruction.rarg) && ir.has_death_point(instruction.rarg, instruction.instruction_number)) {
            occupied.erase(ir.get_assigned_register(instruction.rarg));
        }

        // The following instructions do not need to be assigned a register
        if(instruction.opcode == Opcode::SETPAR ||
           instruction.opcode == Opcode::CMP ||
           instruction.opcode == Opcode::MOV ||
           instruction.opcode == Opcode::SWAP ||
           instruction.opcode == Opcode::WRITE ||
           instruction.opcode == Opcode::WRITENL ||
           instruction.opcode == Opcode::BRA ||
           instruction.opcode == Opcode::BNE ||
           instruction.opcode == Opcode::BEQ ||
           instruction.opcode == Opcode::BLE ||
           instruction.opcode == Opcode::BLT ||
           instruction.opcode == Opcode::BGE ||
           instruction.opcode == Opcode::BGT ||
           instruction.opcode == Opcode::EMPTY) {
             continue;
        }

        // Assign oneself a register
        ir.set_assigned_register(instruction.instruction_number, get_register(instruction, occupied));
        if(!ir.has_death_point(instruction.instruction_number, instruction.instruction_number)) {
            // If an instruction lives and dies in the same spot, the register is used and then is unoccupied again
            occupied.insert(ir.get_assigned_register(instruction.instruction_number));
        }
    }

    // This block has now been colored
    ir.set_colored(block);

    // Create phi moves where necessary
    if(ir.is_branch_back(block)) { // current block is a branch-back block
        implement_phi_copies(block, ir.get_loop_header(block));
    }
    for(const auto& predecessor : ir.get_predecessors(block)) {
        if(ir.is_colored(predecessor)) {
            implement_phi_copies(predecessor, block);
        }
    }
    for(const auto& successor: ir.get_successors(block)) {
        if(ir.is_colored(successor)) {
            implement_phi_copies(block, successor);
        }
    }

    // Color successors
    for(const auto& successor : ir.get_successors(block)) {
        color_block(successor);
    }
}

void RegisterAllocator::implement_phi_copies(const bb_t& block, const bb_t& phi_block) {
    // This is arguably unsafe due to it being a consequence of my parser implementation.
    //
    // Because of how phi functions are created in the parser, right arguments always
    // belong to the non-branching block, left arguments belong to branching blocks.
    //
    // Left arguments also belong to blocks above the phi_block in the case the phi_block is a
    // loop header.
    //
    // Right arguments also belong to branch-back blocks from the phi_block in the case the
    // phi_block is a loop header.

    // If we've propagated the phis already:
    // if(block.propagated) return;

    // While loop header (phi_block) and branch-back block (block)
    if(ir.is_loop_branch_back_related(phi_block, block)) {
        insert_phi_copies(block, phi_block, false);
    } 
    // While loop header (phi_block) and block above it (block)
    else if(ir.is_loop_header(phi_block) && ir.has_one_successor(block) && ir.get_successors(block).at(0) == phi_block) {
        insert_phi_copies(block, phi_block, true);
    }
    // Join block (phi_block) and block branching to it (block)
    else if(ir.has_branch_instruction(block)) {
        insert_phi_copies(block, phi_block, true);
    }
    // Join block (phi_block) and block falling through to it (block)
    else {
        insert_phi_copies(block, phi_block, false);
    }

    // Phis have been propagated to the block.
    // block.propagated = true;
}

void RegisterAllocator::insert_phi_copies(const bb_t& block, const bb_t& phi_block, const bool& left) {
    //TODO: Instead of blindly inserting MOV calls, we should insert them in a proper order:
    // 1. mix of mov and xchg calls
    // 2. const movs
    // We could probably represent this problem as a graph problem. Nodes are registers, edges,
    // with either uni or bi directionality, resemble the need of moving the contents of one register to another.
    // If the edge is bidirectional, this calls for an xchg call.
    // A chain of mov/xchg calls requires finding a node in the graph with no outward edges. This means the register
    // that node represents has unneeded data, and can be safely replaced.
    // What if we have a cycle?
    // For example:
    // blue <- green <- red <- blue 
    // 
    // blue <- green
    // green <- red
    // red <- blue
    //
    // We could solve this given example with:
    // xchg blue, green
    // xchg green, red
    //
    // So it seems to solve a clean cycle, we just need continuous xchg calls. But what if two registers need the same data
    // from another register? Then it gets complicated...

    // Step 1: mov to all registers that have unneeded edges (aka no outgoing edges)
    // Step 2: xchg cycles
    // Step 3: const movs
    std::unordered_map<Register, int> to_out_degree;
    std::map<instruct_t, instruct_t> mov_instructs;
    std::vector<std::pair<instruct_t, instruct_t>> const_movs;
    for(const auto& instruction : ir.get_instructions(phi_block)) {
        if(instruction.opcode == Opcode::DELETED) continue;
        if(instruction.opcode != Opcode::PHI) break;
        if(left) {
            if(ir.is_const_instruction(instruction.larg)) { 
                const_movs.emplace_back(instruction.instruction_number, instruction.larg);
            } else if(ir.get_assigned_register(instruction.instruction_number) == ir.get_assigned_register(instruction.larg)) {
                continue;
            } else {
                if(to_out_degree.find(ir.get_assigned_register(instruction.instruction_number)) == to_out_degree.end()) {
                    to_out_degree[ir.get_assigned_register(instruction.instruction_number)] = 0;
                }

                // "from" instruction
                ++to_out_degree[ir.get_assigned_register(instruction.larg)];

                // "to" instruction
                mov_instructs[instruction.instruction_number] = instruction.larg;
            }
            // ir.add_instruction(block, Opcode::MOV, instruction.instruction_number, instruction.larg); 
        } else { // right
            if(ir.is_const_instruction(instruction.rarg)) { 
                const_movs.emplace_back(instruction.instruction_number, instruction.rarg);
            } else if(ir.get_assigned_register(instruction.instruction_number) == ir.get_assigned_register(instruction.rarg)) {
                continue;
            } else {
                if(to_out_degree.find(ir.get_assigned_register(instruction.instruction_number)) == to_out_degree.end()) {
                     to_out_degree[ir.get_assigned_register(instruction.instruction_number)] = 0;
                }

                // "from" instruction
                ++to_out_degree[ir.get_assigned_register(instruction.rarg)];

                // "to" instruction
                mov_instructs[instruction.instruction_number] = instruction.rarg;
            }
            // ir.add_instruction(block, Opcode::MOV, instruction.instruction_number, instruction.rarg); 
        }
    }

    // Insert mov/xchg calls
    std::map<Register, instruct_t> reg_map;
    for(const auto& pair : mov_instructs) {
        reg_map[ir.get_assigned_register(pair.first)] = pair.first;
    }

    // Step 1: mov to all registers that have unneeded data (aka no outgoing edges)
    std::queue<instruct_t> zero_out_degree_instructions;
    for(const auto& pair : mov_instructs) {
        if(to_out_degree[ir.get_assigned_register(pair.first)] == 0) {
            zero_out_degree_instructions.push(pair.first);
        }
    }
    while(!zero_out_degree_instructions.empty()) {
        instruct_t to_instruct = zero_out_degree_instructions.front();
        zero_out_degree_instructions.pop();
        if (mov_instructs.find(to_instruct) != mov_instructs.end()) {
            instruct_t from_instruct = mov_instructs[to_instruct];
            ir.add_instruction(block, Opcode::MOV, to_instruct, from_instruct);
        
            // Update out-degree of the from_instruct
            --to_out_degree[ir.get_assigned_register(from_instruct)];

            // Remove processed instruction from mov_instructs
            mov_instructs.erase(to_instruct);

            // If from_instruct now has zero out-degree, add to queue
            if(to_out_degree[ir.get_assigned_register(from_instruct)] == 0) {
                zero_out_degree_instructions.push(reg_map[ir.get_assigned_register(from_instruct)]);
            }
        }
    }

    // Step 2: xchg cycles
    while(!mov_instructs.empty()) {
        instruct_t to = mov_instructs.begin()->first;
        instruct_t from = mov_instructs.begin()->second;
        Register end = ir.get_assigned_register(to);
        while(true) {
            ir.add_instruction(block, Opcode::SWAP, to, from); 
            mov_instructs.erase(mov_instructs.find(to));
            to = reg_map[ir.get_assigned_register(from)];
            from = mov_instructs.find(to)->second;
            if(ir.get_assigned_register(from) == end) {
                mov_instructs.erase(mov_instructs.find(to));
                break;
            }
        }
    }

    // Step 3: insert const movs
    for (const auto& pair : const_movs) {
        ir.add_instruction(block, Opcode::MOV, pair.first, pair.second);
    }
}

Register RegisterAllocator::get_register(const Instruction& instruction, const std::unordered_set<Register>& occupied) {
    auto preference = ir.get_instruction_preference(instruction.instruction_number);
    sort(preference.begin(), preference.end(), Preference::sort_by_preference);
    for(const auto& pair : preference) {
        if(occupied.find(pair.first) == occupied.end()) return pair.first;
    }
    ++ir.spill_count;
    return static_cast<Register>(Register::UNASSIGNED + ir.spill_count);
    // throw std::runtime_error{"Ran out of registers"};
}
