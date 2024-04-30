#ifndef LEXER_HPP
#define LEXER_HPP

#include <iostream>
#include <map>
#include <variant>

#include "token.hpp"


enum class TokenType {
    INVALID,        /* Initial state of a token if default initialized. */
    TERMINAL,       /* Refers to terminal symbols of the language. i.e. '<' or '+'. */
    CONSTANT,       /* Refers to constant values, only integers are supported in Tiny. */
    IDENTIFIER,     /* Refers to names for variables and functions. */
};


struct Token {
    TokenType type;
    std::variant<Terminal, int> payload;

    void print();
};


class LexerException : public std::exception {
public:
    LexerException(const std::string& msg);
    const char* what() const noexcept override;
private:
    std::string message;
};


using ident_t = size_t;

class Lexer {
public:
    Lexer(std::istream& in);

    ident_t ident_index = 0;
    Token token;
    void next();

private:
    std::map<std::string, int> identifier_table {
#define KEYWORD(name, encoding) {#encoding, -static_cast<int>(Keyword::name)},
        KEYWORD_LIST
#undef KEYWORD
    };

    std::istreambuf_iterator<char> istream;
    void tokenize_identifier();
    void tokenize_constant();
    void tokenize_terminal();
};



#endif // LEXER_HPP
