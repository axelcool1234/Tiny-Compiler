#ifndef PARSER_HPP
#define PARSER_HPP
// #include "tokenizer.hpp"
#include "intermediaterepresentation.hpp"
#include "lexer.hpp"

class Parser {
private:
    Lexer lexer;
    IntermediateRepresentation ir;

    // Helpers
    template<typename... Args>
    bool is_terminal_kind(const Token& token, Args... args);

    template<typename... Args>
    bool is_identifier_index(const Token& token, Args... args);

    // Parsing
    void variable_declaration();
    // void function_declaration();
    void statement_sequence();

    void statement();
    int expression();
    int term();
    int factor();

public:
    Parser();
    void computation();
};

#endif // PARSER_HPP
