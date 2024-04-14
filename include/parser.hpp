#ifndef PARSER_HPP
#define PARSER_HPP
// #include "tokenizer.hpp"
#include "lexer.hpp"

class Parser {
private:
    Lexer lexer;

    // Helpers
    template<typename... Args>
    bool is_terminal_kind(const Token& token, Args... args);

    template<typename... Args>
    bool is_identifier_index(const Token& token, Args... args);

    // Parsing
    void variable();
    int expression();
    int term();
    int factor();

public:
    Parser();
    void computation();
};

#endif // PARSER_HPP
