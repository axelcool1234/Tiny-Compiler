#ifndef INTERMEDIATEREPRESENTATION_HPP
#define INTERMEDIATEREPRESENTATION_HPP

#include "basicblock.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define REGISTER_LIST \
    REGISTER(RAX, %rax) \
    REGISTER(RBX, %rbx) \
    REGISTER(RCX, %rcx) \
    REGISTER(RDX, %rdx) \
    REGISTER(RSI, %rsi) \
    REGISTER(RDI, %rdi) \
    REGISTER(R8D, %r8) \
    REGISTER(R9D, %r9) \
    REGISTER(R10D, %r10) \
    REGISTER(R14D, %r14) \
    REGISTER(R15D, %r15)

// TODO: Someday, we could add R12D and R13D back again. 
// It has special cases in the assembler, so have been removed.

// %r11 is dedicated as a temporary register
// %rsp is dedicated as the stack pointer
// %rbp is dedicated as the base pointer

enum Register {
#define REGISTER(name, str) name,
    REGISTER_LIST
#undef REGISTER
    LAST,
    UNASSIGNED
};

static const std::vector<std::string> reg_str_list {
#define REGISTER(name, str) #str,
    REGISTER_LIST
#undef REGISTER
};

struct Preference {
    std::unordered_set<instruct_t> affinities;
    std::vector<std::pair<Register, int>> preference;
    Preference();
    static bool sort_by_preference(const std::pair<Register, int> &a, const std::pair<Register, int> &b) {
        return a.second > b.second;
    }
};


class IntermediateRepresentation {
public:
    IntermediateRepresentation();
    void debug() const;
    bool ignore = false;
    bool while_loop = false;
    int spill_count = 0;
    /*
     * Resets the doms vector and recomputes the dominators for every basic
     * block in the basic_blocks vector
     */
    void compute_dominators();

    /*
     * Utilizes the Cooper-Harvey-Kennedy algorithm to determine the closest
     * dominator the two given basic blocks have in common. This function
     * is mainly used to determine the immediate dominator of a basic block
     * given it's two parents. 
     * 
     * @param b1 The first basic block's index.
     * @param b2 The second basic block's index.
     * @return The common dominator between block b1 and block b2.
     */
    bb_t intersect(bb_t b1, bb_t b2) const;

    /*
     * Creates a new block with the given parent. The block's
     * type will be NONE. This function should only be called
     * for creating the first block after the const block.
     * 
     * @param p The new block's parent's index (which should be the const block).
     * @return The index of the new block.
     */
    bb_t new_block(const bb_t& p);

    /*
     * Creates a new block with the given parent and block type.
     *
     * @param p The new block's parent's index.
     * @param t The new block's type.
     * @return The index of the new block.
     */
    bb_t new_block(const bb_t& p, Blocktype t);

    /*
     * Creates a new block with the given parents and immediate dominator. The block's
     * type will be JOIN, given it has two parents. The block will also start with generated
     * phi functions based off the discrepencies in the parent blocks'
     * identifier values.
     *
     * @param p1 The index of the first parent block of the new block.
     * @param p2 The index of the second parent block of the new block.
     * @param idom The immediate dominator of the new block.
     * @return The index of the new block.
     */
    bb_t new_block(const bb_t& p1, const bb_t& p2, const bb_t& idom);

    /*
     * Given an immediate dominator, creates a new block for a function.
     *
     * @param idom The new block's immediate dominator.
     * @param ident_count The current amount of identifiers.
     */ 
    bb_t new_function(const bb_t& idom, const ident_t& ident_count);

    /*
     * Given a block that potentially only contains an EMPTY instruction, change that
     * instruction to the given opcode and arguments, while continuing to use
     * the now changed EMPTY instruction's instruction number. If the block doesn't
     * meet that condition, it'll remain unchanged.
     *
     * @param b The index of the block being changed.
     * @param op The opcode the EMPTY instruction whill change to.
     * @param larg The left argument that the EMPTY instruction's larg will change to.
     * @param rarg The right argument that the EMPTY instruction's rarg will change to.
     * @return Returns the now-changed EMPTY instruction's instruction number upon success, or
     * -1 given that the given block doesn't only contain an EMPTY instruction.
     */
    instruct_t change_empty(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg);

    /*
     * Given a block that potentially only contains an EMPTY instruction, change that
     * instruction to the given opcode and arguments, while continuing to use
     * the now changed EMPTY instruction's instruction number. If the block doesn't
     * meet that condition, it'll remain unchanged.
     *
     * @param b The index of the block being changed.
     * @param op The opcode the EMPTY instruction whill change to.
     * @param larg The left argument that the EMPTY instruction's larg will change to. Also contains the identifier that has
     * been assigned this instruction.
     * @param rarg The right argument that the EMPTY instruction's rarg will change to. Also contains the identifier that has
     * been assigned this instruction.
     * @return Returns the now-changed EMPTY instruction's instruction number upon success, or
     * -1 given that the given block doesn't only contain an EMPTY instruction.
     */
    instruct_t change_empty(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg);

    /*
     * Attempts to add a new instruction to the given block. If a pre-existing instruction with the same
     * opcode and arguments already exists, that instruction's instruction number will be returned instead.
     * If the given block only contains the EMPTY instruction, the given opcode and arguments will replace
     * that instead of creating a new instruction. 
     *
     * @param b The given block's index.
     * @param op The opcode of the new instruction.
     * @param larg The left argument of the new instruction. -1 by default.
     * @param rarg The right argument of the new instruction. -1 by default.
     * @return The instruction number of the instruction, which could either be a common subexpression
     * (and thus not a new instruction), or the replaced EMPTY instruction. Otherwise it'll be the
     * instruction number of the newly created instruction.
     */
    instruct_t add_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg); 

    
    /*
     * Attempts to add a new instruction to the given block. If a pre-existing instruction with the same
     * opcode and arguments already exists, that instruction's instruction number will be returned instead.
     * If the given block only contains the EMPTY instruction, the given opcode and arguments will replace
     * that instead of creating a new instruction. 
     *
     * @param b The given block's index.
     * @param op The opcode of the new instruction.
     * @param larg The left argument of the new instruction. -1 by default. Also contains the identifier assigned
     * this instruction.
     * @param rarg The right argument of the new instruction. -1 by default. Also contains the identifier assigned
     * this instruction.
     * @return The instruction number of the instruction, which could either be a common subexpression
     * (and thus not a new instruction), or the replaced EMPTY instruction. Otherwise it'll be the
     * instruction number of the newly created instruction.
     */
    instruct_t add_instruction(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg);

    /*
     * Attempts to add a new instruction to the given block. If a pre-existing instruction with the same
     * opcode and arguments already exists, that instruction's instruction number will be returned instead.
     * If the given block only contains the EMPTY instruction, the given opcode and arguments will replace
     * that instead of creating a new instruction. 
     *
     * @overload
     */
    instruct_t add_instruction(const bb_t& b, Opcode op, const instruct_t& larg); 

    /*
     * Attempts to add a new instruction to the given block. If a pre-existing instruction with the same
     * opcode and arguments already exists, that instruction's instruction number will be returned instead.
     * If the given block only contains the EMPTY instruction, the given opcode and arguments will replace
     * that instead of creating a new instruction. 
     *
     * @overload
     */
    instruct_t add_instruction(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg);

    /*
     * Attempts to add a new instruction to the given block. If a pre-existing instruction with the same
     * opcode and arguments already exists, that instruction's instruction number will be returned instead.
     * If the given block only contains the EMPTY instruction, the given opcode and arguments will replace
     * that instead of creating a new instruction. 
     *
     * @overload
     */
    instruct_t add_instruction(const bb_t& b, Opcode op); 

    
    /*
     * Attempts to add a new instruction to the given block at the beginning of its instruction vector. 
     * If a pre-existing instruction with the same opcode and arguments already exists, that instruction's 
     * instruction number will be returned instead. If the given block only contains the EMPTY instruction, 
     * the given opcode and arguments will replace that instead of creating a new instruction. 
     *
     * @param b The given block's index.
     * @param op The opcode of the new instruction.
     * @param larg The left argument of the new instruction.
     * @param rarg The right argument of the new instruction.
     * @return The instruction number of the instruction, which could either be a common subexpression
     * (and thus not a new instruction), or the replaced EMPTY instruction. Otherwise it'll be the
     * instruction number of the newly created instruction.
     */
    instruct_t prepend_instruction(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg); 

    /*
     * Attempts to add a new instruction to the given block at the beginning of its instruction vector. 
     * If a pre-existing instruction with the same opcode and arguments already exists, that instruction's 
     * instruction number will be returned instead. If the given block only contains the EMPTY instruction, 
     * the given opcode and arguments will replace that instead of creating a new instruction. 
     *
     * @param b The given block's index.
     * @param op The opcode of the new instruction.
     * @param larg The left argument of the new instruction along with the identifier that has been assigned this
     * instruction.
     * @param rarg The right argument of the new instruction along with the identifier that has been assigned this
     * instruction.
     * @return The instruction number of the instruction, which could either be a common subexpression
     * (and thus not a new instruction), or the replaced EMPTY instruction. Otherwise it'll be the
     * instruction number of the newly created instruction.
     */
    instruct_t prepend_instruction(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg);

   /*
    * Given a while loop's scope from its header block to its branch back block,
    * this function generates phi functions based off the differences in identifier values
    * of the loop header block and branch back block. Then, the instruction numbers of the
    * new phi functions are propagated across the while loop's scope, replacing old identifier
    * values with the new phi functions' instruction numbers.
    *
    * @param loop_header The while loop's header.
    * @param branch_back The while loop's branch block.
    */
    void generate_phi(const bb_t& loop_header, const bb_t& branch_back);
    void update_phi(const bb_t& loop_header, const bb_t& branch_back);

    /*
     * Given a vector of tuples containing information about identifiers that should be changed and what their value should be
     * changed to, this function will update those identifiers with the given new values, starting from the given current block 
     * and down through the control flow graph until there are no blocks to travel to.
     * 
     * @param curr_block The first block to be changed.
     * @param changed_idents First argument is the index of the identifier in any basic block's identifier values vector. Second
     * argument is the new instruction number it should be changed to. Third argument is the old instruction number that should be changed to
     * the new instruction number.
     */
    void update_ident_vals_loop(const bb_t& curr_block, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents);

    // OUTDATED (Kept for now)
    void update_ident_vals_until(bb_t curr_block, bb_t stop_block, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents);

    /*
     * Given a block and a tuple containing information about identifiers that should be changed and what their value should be changed to,
     * this function will update those identifiers with the given new values for this block.
     *
     * @param b The given block.
     * @param changed_idents First argument is the index of the identifier in any basic block's identifier values vector. Second
     * argument is the new instruction number it should be changed to. Third argument is the old instruction number that should be changed to
     * the new instruction number.
     * @param skip_phi If true, phi instructions are not changed. Else, they are changed like every other instruction.
     */
    void update_ident_vals(const bb_t& b, const std::vector<std::tuple<int, instruct_t, instruct_t>>& changed_idents, const bool& skip_phi);

    /*
     * Returns the instruction number of the given block's first instruction.
     * If the given block is completely empty, an EMPTY instruction is added to
     * it and the instruction number of the new EMPTY instruction is returned.
     *
     * @param b The given block's index.
     * @return The instruction number of the given block's first instruction.
     */ 
    instruct_t first_instruction(const bb_t& b);

    /*
     * Searches for potential common subexpressions with the given opcode, left argument,
     * and right argument. First searches through the given block, then through the given
     * block's immediate dominator, then through that dominator's immediate dominator, etc.
     *
     * @param b The given block's index.
     * @param op The opcode being looked for. If CSEs are not possible for this opcode, this
     * function immediately returns -1.
     * @param larg The left argument being looked for.
     * @param rarg The right argument being looked for.
     * @return Returns the instruction number of an instruction that matches the given opcode,
     * left argument, and right argument. If no match is found, or the given opcode can't have
     * CSEs, -1 is returned. 
     */
    instruct_t search_cse(const bb_t& b, Opcode op, const instruct_t& larg, const instruct_t& rarg);

    /*
     * Returns the instruction number the given identifier is assigned to in the given block.
     *
     * @param b The given block's index.
     * @param ident The given identifier's index.
     * @return The value (aka instruction number) of the given identifier in the given block.
     */ 
    instruct_t get_ident_value(const bb_t& b, const ident_t& ident);

    /*
     * Changes the given identifier's instruction number in the given block.
     *
     * @param b The given block's index.
     * @param ident The given identifier's index.
     * @param instruct The instruction number that'll be assigned to the given
     * identifier in the given block.
     */
    void change_ident_value(const bb_t& b, const ident_t& ident, const instruct_t& instruct);

    /*
     * Sets the given block's will_return bool to true.
     * 
     * @param b The given block's index.
     */ 
    void set_return(const bb_t& b);

    /* 
     * Returns the given block's will_return bool value.
     *
     * @param b The given block's index.
     */
    bool will_return(const bb_t& b) const;

    /* 
     * Sets the opcode (aka condition) for the given block's branch instruction. This
     * function is used for comparison branch instructions, and for "branch always"
     * instructions.
     *
     * @param b The given block's index.
     * @param op The opcode that'll be assigned to the branch instruction.
     * @param larg Either a comparison instruction number or an instruction number to
     * "branch always" to.
     */ 
    void set_branch_cond(const bb_t& b, Opcode op, const instruct_t& larg);

    /* 
     * Sets the opcode (aka condition) for the given block's branch instruction. This
     * function is used for comparison branch instructions, and for "branch always"
     * instructions.
     *
     * @param b The given block's index.
     * @param op The opcode that'll be assigned to the branch instruction.
     * @param larg A comparison instruction/branch instruction along with the identifier
     * that has been assigned that instruction.
     */ 
    void set_branch_cond(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg);

    /*
     * Sets the instruction number the given block's branch instruction will branch to
     * upon success.
     *
     * @param b The given block's index.
     * @param rarg The instruction number the given block's branch instruction will branch
     * to upon success.
     */ 
    void set_branch_location(const bb_t& b, const instruct_t& rarg);

    /*
     * Creates a dotlang representation of the IR - used for debugging.
     *
     * @return The string that contains the dotlang code representing the
     * IR.
     */
    std::string to_dotlang() const;

    void init_live_ins();

    void insert_death_point(const instruct_t& instruct, const instruct_t& death_point);

    void erase_live_in(const bb_t& b, const instruct_t& instruct);
    void insert_live_in(const bb_t& b, const instruct_t& instruct);

    /*
     * Attempts to propagate the live SSA instructions from the given block's
     * successors. If any of the successors have not been liveness analyzed,
     * the live SSA instructions are not propagated.
     *
     * @param b The given block's index.
     * @return Returns true if the live SSA instructions of the given block's successors
     * have been successfully propagated to the given block. Returns false if one of
     * the successors haven't been analyzed yet.
     */
    bool propagate_live_ins(const bb_t& b);

    /*
     * Returns the index of the immediate dominator of the given block.
     *
     * @param b The given block's index.
     * @return The index of the given block's immediate dominator.
     */ 
    const bb_t& get_idom(const bb_t& b) const;
    const Blocktype& get_block_type(const bb_t& b) const;
    const std::vector<BasicBlock>& get_basic_blocks() const;
    std::unordered_set<instruct_t>& get_live_ins(const bb_t& b);
    std::unordered_map<instruct_t, std::unordered_set<instruct_t>>& get_death_points();
    const Register& get_assigned_register(const instruct_t& instruct) const;
    const std::vector<instruct_t>& get_predecessors(const bb_t& b) const;
    const std::vector<instruct_t>& get_successors(const bb_t& b) const;
    const std::vector<Instruction>& get_instructions(const bb_t& b) const;
    const instruct_t& get_loop_header(const bb_t& b) const;
    const instruct_t& get_branch_back(const bb_t& b) const;
    const Blocktype& get_type(const bb_t& b) const;
    const Instruction& get_branch_instruction(const bb_t& b) const;
    const int& get_const_value(const instruct_t& instruct) const;
    const std::vector<std::pair<Register, int>>& get_instruction_preference(const instruct_t& instruct);

    void constrain(const instruct_t& instruct, const bb_t& b, const Register& reg, const bool& propagate);
    void dislike(const instruct_t& instruct, const bb_t& b, const Register& reg, const bool& propagate);
    void prefer(const instruct_t& instruct, const Register& reg, const bool& propagate);

    void set_colored(const bb_t& b);
    void set_analyzed(const bb_t& b); 
    void set_propagated(const bb_t& b); 
    void set_emitted(const bb_t& b); 
    void set_assigned_register(const instruct_t& instruct, const Register& reg);

    bool has_assigned_register(const instruct_t& instruct) const;
    bool has_preference(const instruct_t& instruct) const;
    bool has_death_point(const instruct_t& instruct, const instruct_t& death_point) const;
    bool has_branch_instruction(const bb_t& b) const;
    bool has_zero_successors(const bb_t& b) const;
    bool has_one_successor(const bb_t& b) const;
    bool has_two_successors(const bb_t& b) const;
    bool has_zero_predecessors(const bb_t& b) const;
    bool has_one_predecessor(const bb_t& b) const;
    bool has_two_predecessors(const bb_t& b) const;

    bool is_loop_branch_back_related(const bb_t& loop_header, const bb_t& branch_back) const;
    bool is_live_instruction(const bb_t& b, const instruct_t& instruct) const;
    bool is_const_instruction(const instruct_t& instruct) const;
    bool is_valid_instruction(const instruct_t& instruct) const;
    bool is_undefined_instruction(const instruct_t& instruct) const;
    bool is_loop_header(const bb_t& b) const;
    bool is_branch_back(const bb_t& b) const;
    bool is_colored(const bb_t& b) const;
    bool is_analyzed(const bb_t& b) const;
    bool is_propagated(const bb_t& b) const;
    bool is_emitted(const bb_t& b) const;
    bool is_const_block(const bb_t& b) const;

    void commit_while(const bb_t& curr_block, const bb_t& loop_header, const bb_t& branch_back, std::map<std::string, ident_t>& identifier_table);
    void cse_replace(const bb_t& curr_block, const bb_t& branch_back, const instruct_t& replacer, const instruct_t& to_delete);
private:
// Should be private:
    /*
     * This vector contains the basic blocks of the IR.
     * The bb_t type refers to the indices of this vector.
     */
    std::vector<BasicBlock> basic_blocks;

    /*
     * This records the amount of instructions that have been added to the
     * IR's basic blocks. This is used to get the next available instruction
     * number that can be used for a new instruction.
     */ 
    int instruction_count = 0;

    /*
     * This vector contains the immediate dominators of every basic block
     * in the basic_blocks vector. For example, the nth basic block's
     * immediate dominator would be doms[n].
     */
    std::vector<bb_t> doms;
    std::vector<std::unordered_set<instruct_t>> live_ins;
    std::unordered_map<instruct_t, Preference> preference_list{};
    std::unordered_map<instruct_t, int> const_instructions {{0, 0}};
    std::unordered_map<instruct_t, Register> assigned_registers;
    std::unordered_map<instruct_t, std::unordered_set<instruct_t>> death_points;
    /* Helpers */
    bb_t new_block_helper(const bb_t& p1, const bb_t& p2, const bb_t& idom, Blocktype t);
    instruct_t add_instruction_helper(const bb_t& b, Opcode op, const std::pair<instruct_t, ident_t>& larg, const std::pair<instruct_t, ident_t>& rarg, const bool& prepend);
    Preference& get_preference(const instruct_t& instruct);
    void establish_affinity_group(const instruct_t& i1, const instruct_t& i2, const instruct_t& i3);

    /* Debug */
    void print_const_instructions() const;
    void print_live_ins() const;
    void print_leaf_blocks() const;
    void print_unanalyzed_blocks() const;
    void print_uncolored_blocks() const;
    void print_unemitted_blocks() const;
    void print_colored_instructions() const;
    void print_death_points() const;
    void print_preferences() const;
    void print_affinity_groups() const;
};

#endif // INTERMEDIATEREPRESENTATION_HPP
