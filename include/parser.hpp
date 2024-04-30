#ifndef PARSER_HPP
#define PARSER_HPP
#include "intermediaterepresentation.hpp"
#include "lexer.hpp"

class Parser {
private:
    Lexer lexer;
    IntermediateRepresentation ir;
    const int const_block = 0;

    /* Helpers */
    template<typename... Args>
    bool is_terminal_kind(const Token& token, Args... args);

    template<typename... Args>
    bool is_identifier_index(const Token& token, Args... args);

    Opcode terminal_to_opcode(const Terminal& terminal);

    /* Parsing */
    // Declarations
    void variable_declaration();
    // void function_declaration();

    // Statements
    void main_statement_sequence(bb_t& curr_block);
    void then_statement_sequence(bb_t& curr_block);
    void else_statement_sequence(bb_t& curr_block);
    void while_statement_sequence(bb_t& curr_block);
    void statement(bb_t& curr_block);
    void let_statement(const bb_t& curr_block);
    void func_statement(bb_t& curr_block);
    void if_statement(bb_t& curr_block);
    void while_statement(bb_t& curr_block);
    void return_statement(bb_t& curr_block);

    void relation(const bb_t& curr_block);
    instruct_t expression(const bb_t& curr_block);
    instruct_t term(const bb_t& curr_block);
    instruct_t factor(const bb_t& curr_block);

public:
    Parser();
    void computation();
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
