#ifndef PARSER_HPP
#define PARSER_HPP
#include "intermediaterepresentation.hpp"
#include "lexer.hpp"

enum class Relation {
 TRUE,
 FALSE,
 NEITHER
};

class Parser {
public:
    Parser();

    /*
     * Given an stdin of Tiny code, parses given code into an intermediate representation.
     */ 
     void parse();

    /*
     * Prints the intermediate representation of the parsed Tiny code.
     */
     void print();

     /*
      * Releases the intermediate representation object via move semantics.
      */ 
     IntermediateRepresentation release_ir();
private:
    Lexer lexer;
    IntermediateRepresentation ir;
    const int const_block = 0;

    /* Helpers */
    /*
     * Checks if the given token matches any of the given terminals/keywords.
     *
     * @param token The given token being checked.
     * @param args The set of terminals/keywords the token is being checked against.
     * @return true if the given token is of type T and matches one of the given terminals/keywords.
     */
    template<typename T, typename... Args>
    bool token_is(const Token& token, const T& expected, const Args&... args);

    /*
     * Checks if the given token is of type T.
     *
     * @param token The given token being checked.
     * @return true if the given token is of type T.
     */
    template<typename T>
    bool token_is(const Token& token);

   /*
    * Checks the type of the current token. If it isn't the specified type T, an exception
    * is raised with the expected token type.
    */
    template<typename T>
    void assert_type();

   /*
    * Checks that the current token in the lexer is the given keyword/terminal. If it isn't,
    * an exception is raised with the expected token and the current token in the lexer. 
    * Calls lexer.next() on success.
    *
    * @param expected The given keyword/terminal the lexer's token should be.
    */
    template<typename T>
    void match(const T& expected);   

   /*
    * Checks that the current token is of type T. If it isn't, an exception is raised with 
    * the expected type. If it is, an object of type T is retrieved from the token's payload. 
    * Calls lexer.next() on success.
    *
    * @return an object of type T retrieved from the token's payload is return upon success.
    */ 
    template<typename T>
    void match();

   /*
    * Checks that the current token is of type T. If it isn't,
    * an exception is raised with the expected type. If it is, an object of
    * type T is retrieved from the token's payload. Calls lexer.next() and then returns
    * the object.
    *
    * @param msg The message given to the raised exception if the lexer' token isn't of type T.
    * @return an object of type T retrieved from the token's payload is return upon success.
    */ 
    template<typename T>
    T match_return();

    /*
     * Translates a terminal to an appropriate opcode that'd be used. Note,
     * that the opposite opcode is produced due to branching rules being flipped
     * in Assembly.
     *
     * @param terminal The given terminal.
     * @return The opcode appropriate for the given terminal.
     */
    Opcode terminal_to_opcode(const Terminal& terminal);

    /*
     * Given a terminal (which is then translated to a comparison) and two constants, 
     * returns the result of the comparison.
     *
     * @param terminal The given terminal.
     * @param larg The given left argument.
     * @param rarg The given right argument.
     * @return The result of comparing larg and rarg.
     */
    bool terminal_cmp(const Terminal& terminal, const int& larg, const int& rarg);

    /* Computation */
    void computation();

    /* Declarations */
    void variable_declaration();

    bb_t function_declaration();
    void formal_parameters(bb_t& func_block);
    std::vector<ident_t> formal_parameters();
    instruct_t function_body(const std::vector<ident_t>& formal_params, const std::vector<instruct_t>& func_first_instructs);

    /* Statement Sequences */
    /*
     * Continually parses statements until the given keywords/terminals are reached.
     * If the ir is in ignore mode, a fake dummy block is passed to the statement call to
     * prevent the curr_block from being affected. Else, the curr_block is passed to the
     * statement call, and if that statement is gaurunteed to return, then the ir's ignore 
     * mode is activated.
     *
     * @param curr_block The current block of the ir.
     * @param args a variadic amount of terminals and/or keywords that stop the statement 
     * parsing when reached.
     */
    template<typename... Args>
    void statement_sequence(bb_t& curr_block, Args... args);
    bool statement(bb_t& curr_block); // returns true if the statement will always return.

    // "let" statement
    void let_statement(const bb_t& curr_block);

    // "function" statement
    std::pair<instruct_t, ident_t> function_statement(const bb_t& curr_block);
    std::pair<instruct_t, ident_t> predefined_function_statement(const bb_t& curr_block);

    // "if" statement
    bool if_statement(bb_t& curr_block); // returns true if the if statement will always return.
    void join(bb_t& curr_block, const bb_t& then_block, const bb_t& else_block);

    // "while" statement
    bool while_statement(bb_t& curr_block);
    void branch(bb_t& curr_block, const bb_t& while_block);

    // "return" statement
    void return_statement(bb_t& curr_block);

    // relations
    Relation relation(const bb_t& curr_block, const bool& if_statement);

    // Base parsing
    std::pair<instruct_t, ident_t> expression(const bb_t& curr_block);
    std::pair<instruct_t, ident_t> term(const bb_t& curr_block);
    std::pair<instruct_t, ident_t> factor(const bb_t& curr_block);
};

class ParserException : public std::exception {
private:
    std::string message;
public:
    ParserException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

#endif // PARSER_HPP
