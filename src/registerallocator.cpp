#include "registerallocator.hpp"
#include <ranges>
#include <unordered_set>
#include <stdexcept>

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
        for(const auto& instruction : ir.get_instructions(ir.get_loop_header(block))) {
            if(instruction.opcode != Opcode::PHI) break;
            if(ir.is_const_instruction(instruction.rarg)) continue;
            ir.insert_live_in(block, instruction.rarg);
        }
    } 
    // Get liveness from loop header you're entering into for the first time
    else if(ir.has_one_successor(block) && ir.is_loop_header(ir.get_successors(block).at(0))) {
        for(const auto& instruction : ir.get_instructions(ir.get_successors(block).at(0))) {
            if(instruction.opcode != Opcode::PHI) break;
            if(ir.is_const_instruction(instruction.larg)) continue;
            ir.insert_live_in(block, instruction.larg);
        }
    }
    // Get liveness from JOIN block (as block branching to it)
    else if(ir.has_one_successor(block) && ir.has_branch_instruction(block)) {
        for(const auto& instruction : ir.get_instructions(ir.get_successors(block).at(0))) {
            if(instruction.opcode != Opcode::PHI) break;
            if(ir.is_const_instruction(instruction.larg)) continue;
            ir.insert_live_in(block, instruction.larg);
        }
    } 
    // Get liveness from JOIN block (as block falling through to it)
    else if(ir.has_one_successor(block)) {
        for(const auto& instruction : ir.get_instructions(ir.get_successors(block).at(0))) {
            if(instruction.opcode != Opcode::PHI) break;
            if(ir.is_const_instruction(instruction.rarg)) continue;
            ir.insert_live_in(block, instruction.rarg);
        }
    } 
    
    // Determine points of death and liveness of SSA instructions at the beginning of the block.
    for(const auto& instruction : ir.get_instructions(block) | std::views::reverse) {
        // When something is born, everything beyond this point will not have it as a living SSA instruction. 
        ir.erase_live_in(block, instruction.instruction_number);

        // Ignore arguments of phi instructions since the move/swap calls will be in the predecessors.
        // Also ignore branch instructions as they don't require registers to work correctly in x86.
        if(instruction.opcode == Opcode::PHI ||
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

        // When something dies, everything beyond this point will have it as a living SSA instruction.
        // An instruction is a candidate for death if:
        // It is not invalid (ie doesn't exist or the special "0" instruction for undefined user identifiers)
        // It is not a constant
        // It is not alive at this point.
        if(ir.is_valid_instruction(instruction.larg) && !ir.is_const_instruction(instruction.larg) && !ir.is_live_instruction(block, instruction.larg)) {
            ir.insert_live_in(block, instruction.larg);
            ir.insert_death_point(instruction.larg, instruction.instruction_number);
        }
        if(ir.is_valid_instruction(instruction.rarg) && !ir.is_const_instruction(instruction.rarg) && !ir.is_live_instruction(block, instruction.rarg)) {
            ir.insert_live_in(block, instruction.rarg);
            ir.insert_death_point(instruction.rarg, instruction.instruction_number);
        }
    }

    // This block has now been analyzed.
    ir.set_analyzed(block);

    // Analyze predecessor blocks.
    for(const auto& predecessor : ir.get_predecessors(block)) {
        analyze_block(predecessor);
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
        if(instruction.opcode != Opcode::PHI) break;
        ir.set_assigned_register(instruction.instruction_number, get_register(instruction, occupied));
        occupied.insert(ir.get_assigned_register(instruction.instruction_number));
    }

    // Assign registers
    for(const auto& instruction : ir.get_instructions(block)) {
        if(instruction.opcode == Opcode::PHI || instruction.opcode == Opcode::EMPTY) continue;

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
        occupied.insert(ir.get_assigned_register(instruction.instruction_number));
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
        for(const auto& instruction : ir.get_instructions(phi_block)) {
            if(instruction.opcode != Opcode::PHI) break; 
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.rarg 
            ir.add_instruction(block, Opcode::MOV, instruction.instruction_number, instruction.rarg); 
        }
    } 
    // While loop header (phi_block) and block above it (block)
    else if(ir.is_loop_header(phi_block) && ir.has_one_successor(block) && ir.get_successors(block).at(0) == phi_block) {
        for(const auto& instruction : ir.get_instructions(phi_block)) {
            if(instruction.opcode != Opcode::PHI) break;
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.larg
            ir.add_instruction(block, Opcode::MOV, instruction.instruction_number, instruction.larg);
        }
    }
    // Join block (phi_block) and block branching to it (block)
    else if(ir.has_branch_instruction(block)) {
        for(const auto& instruction : ir.get_instructions(phi_block)) {
            if(instruction.opcode != Opcode::PHI) break;
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.larg
            ir.add_instruction(block, Opcode::MOV, instruction.instruction_number, instruction.larg);
        }
    }
    // Join block (phi_block) and block falling through to it (block)
    else {
        for(const auto& instruction : ir.get_instructions(phi_block)) {
            if(instruction.opcode != Opcode::PHI) break;
            // Make instruction in block: MOV phi_block.instruction_number, phi_block.rarg
            ir.add_instruction(block, Opcode::MOV, instruction.instruction_number, instruction.rarg);
        }
    }

    // Phis have been propagated to the block.
    // block.propagated = true;
}

Register RegisterAllocator::get_register(const Instruction& instruction, const std::unordered_set<Register>& occupied) {
    for(int reg = RAX; reg < Register::LAST; ++reg) {
        if(occupied.find(static_cast<Register>(reg)) == occupied.end()) return static_cast<Register>(reg);
    }
    throw std::runtime_error{"Ran out of registers"};
}
