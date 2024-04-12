#ifndef PARSER_HPP
#define PARSER_HPP
#include "tokenizer.hpp"

class Parser {
private:
    TokenIterator it;
    Token token;

    // Helpers
    template<typename... Args>
    bool is_terminal_kind(const Token& token, Args... args);

    template<typename... Args>
    bool is_identifier_address(const Token& token, Args... args);

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
