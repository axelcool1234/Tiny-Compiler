#ifndef PARSER_HPP
#define PARSER_HPP
// #include "tokenizer.hpp"
#include "intermediaterepresentation.hpp"
#include "lexer.hpp"

class Parser {
private:
    Lexer lexer;
    IntermediateRepresentation ir;
    const int const_block = 0;

    // Helpers
    template<typename... Args>
    bool is_terminal_kind(const Token& token, Args... args);

    template<typename... Args>
    bool is_identifier_index(const Token& token, Args... args);

    // Parsing
    void variable_declaration();
    // void function_declaration();
    void statement_sequence(const bb_t& curr_block);
    void statement(const bb_t& curr_block);
    void let_statement(const bb_t& curr_block);

    instruct_t expression(const bb_t& curr_block);
    instruct_t term(const bb_t& curr_block);
    instruct_t factor(const bb_t& curr_block);

public:
    Parser();
    void computation();
};

#endif // PARSER_HPP
