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

    /*
     * Prints the token's information based off what type it is.
     */
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
    /*
     * Stores an integer to identify keywords, functions, and variables.
     * Stores KEYWORDS as non-positive enum values. Stores user functions and variables as positive values.
     */ 
    std::map<std::string, int> identifier_table {
#define KEYWORD(name, encoding) {#encoding, -static_cast<int>(Keyword::name)},
        KEYWORD_LIST
#undef KEYWORD
    };

    std::istreambuf_iterator<char> istream;

    /* 
     * Creates an identifier token by iterating character by character until a non alphanumeric character is found.
     */
    void tokenize_identifier();

    /*
     * Creates a constant token by iterating character by character until a non digit character is found.
     */
    void tokenize_constant();

    /*
     * Creates a terminal token by checking for a 2-char terminal, and if invalid, checks for a 1-char terminal.
     * If it isn't either, the token's type is changed to TokenType::INVALID.
     */
    void tokenize_terminal();
};



#endif // LEXER_HPP
