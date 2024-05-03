#ifndef PARSER_HPP
#define PARSER_HPP
#include "intermediaterepresentation.hpp"
#include "lexer.hpp"

class Parser {
public:
    Parser();

    /*
     * Given an stdin of Tiny code, parses into an intermediate representation of the Tiny code.
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
    bool is_terminal(const Token& token, Args... args);

    /*
     * Checks if the given token is of type TERMINAL.
     * 
     * @param token The given token being checked.
     * @return true if the given token is of type TERMINAL.
     */ 
    bool is_terminal(const Token& token);

    /*
     * Checks if the given token matches any of the given keywords.
     *
     * @param token The given token being checked.
     * @param args The set of keywords the token is being checked against.
     * @return true if the given token is of type KEYWORD_IDENTIFIER and matches one of the given keywords.
     */ 
    template<typename... Args>
    bool is_keyword_identifier(const Token& token, Args... args);

    /*
     * Checks if the given token is of type KEYWORD_IDENTIFIER.
     *
     * @param token The given token being checked.
     * @return true if the given token is of type KEYWORD_IDENTIFIER.
     */ 
    bool is_keyword_identifier(const Token& token);

    /*
     * Checks if the given token is of type USER_IDENTIFIER.
     *
     * @param token The given token being checked.
     * @return true if the given token is of type USER_IDENTIFIER.
     */
    bool is_user_identifier(const Token& token);

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
     * Checks that the current token in the lexer is the given terminal. If it isn't,
     * an exception is raised with the given message. Calls lexer.next() on success.
     *
     * @param t The given terminal the lexer's token should be.
     * @param msg The message given to the raised exception if the lexer's token isn't the given terminal.
     */
    void check_and_next(Terminal t, const std::string& msg);

    /*
     * Checks that the current token in the lexer is the given keyword. If it isn't,
     * an exception is raised with the given message. Calls lexer.next() on success.
     *
     * @param k The given keyword the lexer's token should be.
     * @param msg The message given to the raised exception if the lexer's token isn't the given keyword.
     */
    void check_and_next(Keyword k,  const std::string& msg);

    /*
     * Checks that the current token is of type USER_IDENTIFIER. If it isn't,
     * an exception is raised with the given message. Calls lexer.next() on success.
     *
     * @param msg The message given to the raised exception if the lexer's token isn't of type USER_IDENTIFIER.
     */
    void check_and_next(const std::string& msg);

    /*
     * Checks that the current token is of type T. If it isn't,
     * an exception is raised with the given message. If it is, an object of
     * type T is retrieved from the token's payload. Calls lexer.next() and then returns
     * the object.
     *
     * @param msg The message given to the raised exception if the lexer' token isn't of type T.
     * @retun an object of type T retrieved from the token's payload is return upon success.
     */ 
    template<typename T>
    T check_and_next_return(const std::string& msg);

    /* Computation */
    void computation();
    void main();
    void lbrace();
    void rbrace();
    void period();

    /* Declarations */
    bb_t variable_declaration();
    void var();
    void variable_identifier();
    void semicolon();

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
    ident_t let_ident();
    void let_assign();

    void func_statement(bb_t& curr_block);

    // "if" statement
    void if_statement(bb_t& curr_block);
    void then();
    void fi();
    void join(bb_t& curr_block, const bb_t& then_block, const bb_t& else_block, const bb_t& og_else_block);

    // "while" statement
    void while_statement(bb_t& curr_block);
    void while_do();
    void while_od();
    void branch(bb_t& curr_block, const bb_t& while_block);

    // "return" statement
    void return_statement(bb_t& curr_block);

    // relations
    void relation(const bb_t& curr_block);
    Terminal relation_op();

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
