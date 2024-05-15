#ifndef LEXER_HPP
#define LEXER_HPP

#include <iostream>
#include <map>
#include "token.hpp"

class LexerException : public std::exception {
public:
    LexerException(const std::string& msg);
    const char* what() const noexcept override;
private:
    std::string message;
};


using ident_t = ssize_t;

class Lexer {
public:
    Lexer(std::istream& in);

    ident_t ident_index = 0;
    Token token;
    std::string last_ident_string;
    /* 
     * Consumes a variable amount of characters from the input to produce the next valid token.
     */ 
    void next();

    /*
     * Wipes the lexer's identifier_table (keeping reserved keywords) and ident_index.
     * 
     * @return A vector of all of the removed keys from the identifier table.
     */ 
    std::vector<std::string> wipe();

    /*
     * Given a vector of strings of identifiers, insert them into the identifier_table
     *
     * @param ident_strings A vector of strings of identifiers.
     */ 
    void insert_ident(const std::vector<std::string>& ident_strings);
    
private:
    /*
     * Stores an integer to identify keywords, functions, and variables.
     * Stores KEYWORDS as non-positive enum values. Stores user functions and variables as positive values.
     */ 
    std::map<std::string, ident_t> identifier_table {
#define KEYWORD(name, encoding) {#encoding, -static_cast<ident_t>(Keyword::name)},
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
