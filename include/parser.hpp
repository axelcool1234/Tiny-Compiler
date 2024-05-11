#ifndef PARSER_HPP
#define PARSER_HPP
#include "intermediaterepresentation.hpp"
#include "lexer.hpp"

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
private:
    Lexer lexer;
    IntermediateRepresentation ir;
    const int const_block = 0;

    /* Helpers */

    /*
     * Checks if the given token matches any of the given terminals.
     *
     * @param token The given token being checked.
     * @param args The set of terminals the token is being checked against.
     * @return true if the given token is of type TERMINAL and matches one of the given terminals.
     */
    template<typename... Args>
    bool token_is(const Token& token, const Terminal& terminal, Args... args);

    /*
     * Checks if the given token matches any of the given keywords.
     *
     * @param token The given token being checked.
     * @param args The set of keywords the token is being checked against.
     * @return true if the given token is of type KEYWORD and matches one of the given keywords.
     */
    template<typename... Args>
    bool token_is(const Token& token, const Keyword& keyword, Args... args);

    /*
     * Checks if the given token is of type T.
     *
     * @param token The given token being checked.
     * @return true if the given token is of type T.
     */
    template<typename T>
    bool token_is(const Token& token);

    /*
     * Continually parses statements until one of the given keywords are reached.
     *
     * @param curr_block The current block being worked on by the parser.
     */
    template<typename... Args>
    void keyword_sequence_helper(bb_t& curr_block, Args... args);

    /*
     * Continually parses statements until one of the given terminals are reached.
     *
     * @param curr_block The current block being worked on by the parser.
     */
    template<typename... Args>
    void terminal_sequence_helper(bb_t& curr_block, Args... args);

    /*
     * Parses a statement and then parses a semicolon.
     *
     * @param curr_block The current block being worked on by the parser.
     */
    void statement_handler(bb_t& curr_block);

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
    * Checks the type of the current token. If it isn't the specified type T, an exception
    * is raised with the expected token type.
    */
    template<typename T>
    void assert_type();

   /*
    * Checks that the current token in the lexer is the given keyword. If it isn't,
    * an exception is raised with the expected token. Calls lexer.next() on success.
    *
    * @param k The given keyword the lexer's token should be.
    */
    void match(Keyword k);

   /*
    * Checks that the current token in the lexer is the given terminal. If it isn't,
    * an exception is raised with the expected token. Calls lexer.next() on success.
    *
    * @param t The given terminal the lexer's token should be.
    */
    void match(Terminal t);

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

    /* Computation */
    void computation();

    /* Declarations */
    bb_t variable_declaration();
    void function_declaration();

    /* Statement Sequences */
    void main_statement_sequence(bb_t& curr_block);
    void then_statement_sequence(bb_t& then_block);
    void else_statement_sequence(bb_t& else_block);
    void while_statement_sequence(bb_t& while_block);

    /* Statements */
    void statement(bb_t& curr_block);

    // "let" statement
    void let_statement(const bb_t& curr_block);

    void func_statement(bb_t& curr_block);

    // "if" statement
    void if_statement(bb_t& curr_block);
    void join(bb_t& curr_block, const bb_t& then_block, const bb_t& else_block, const bb_t& og_else_block);

    // "while" statement
    void while_statement(bb_t& curr_block);
    void branch(bb_t& curr_block, const bb_t& while_block);

    // "return" statement
    void return_statement(bb_t& curr_block);

    // relations
    void relation(const bb_t& curr_block);

    // Base parsing
    instruct_t expression(const bb_t& curr_block);
    instruct_t term(const bb_t& curr_block);
    instruct_t factor(const bb_t& curr_block);
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
