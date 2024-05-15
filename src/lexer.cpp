#include "lexer.hpp"
#include "token.hpp"
#include <iterator>

/*
 *  Deterministic Finite Automata of our Lexer
 *              [-------------------------------------START----------------------------------------]
 *       [a-z] /           |                   |                     |           |                 \ [0-9]
 *            /            | [;()*+-/.]        |[!]                  |[>]        | [<]              \ 
 *  [ IDENTIFIER ]   [ 1-char TERMINAL ]     ( no accept )   [ > TERMINAL ] [ < TERMINAL ]      [ CONSTANT ]
 *      | ^                                    | [=]                 | [=]      | [-=]               | ^
 *      |/ [a-z0-9]                            .-----> [ 2-char TERMINAL ] <----.                    |/ [0-9] */

std::vector<std::string> Lexer::wipe() {
    std::vector<std::string> removed_keys;    
    for (auto it = identifier_table.begin(); it != identifier_table.end(); ++it) {
        if (it->second >= 0)
            removed_keys.push_back(it->first);
    }
    for (const auto& key : removed_keys) {
        identifier_table.erase(key);
    }
    return removed_keys;
}

void Lexer::insert_ident(const std::vector<std::string>& ident_strings) {
    for(const auto& str : ident_strings) {
        identifier_table[str] = ident_index; 
        ++ident_index;
    }
}

void Lexer::next() {
    // Always skip whitespace
    while (std::isspace(*istream)) { ++istream; }

    // If we are at EOF, then always return invalid tokens
    if (istream == std::istreambuf_iterator<char>{}) {
        token = Invalid();
        return;
    }

    if (std::isalpha(*istream)) { // alphas (for the first char) belong to identifier tokens.
        tokenize_identifier();
    } else if (std::isdigit(*istream)) { // digits (for the first char) belong to constant tokens.
        tokenize_constant();
    } else { // symbols are for terminals.
        tokenize_terminal();
    }

    if (std::holds_alternative<Invalid>(token)) {
        throw LexerException("Failed to tokenize given input. Cannot identify text as either an identifier, constant or terminal!");
    }
}


void Lexer::tokenize_identifier() {
    std::string lexeme;

    // identifiers can have lowercase alphas or digits (just lowercase alphas for the first char)
    while (std::isalpha(*istream) || std::isdigit(*istream)) {
        lexeme.push_back(*(istream++));
    }

    // Check if this identifier has been seen before
    if(!identifier_table.contains(lexeme)) {
        identifier_table[lexeme] = ident_index;
        ++ident_index;
    }
    const int& ident_value = identifier_table.at(lexeme);     

    if(ident_value >= 0) {
        token = static_cast<ident_t>(ident_value);
    } else {
        token = static_cast<Keyword>(-ident_value);
    }
    last_ident_string = std::move(lexeme);
};


void Lexer::tokenize_constant()
{
    int val{};

    // Read until a non-digit is found (builds an integer for the const token)
    do {
        val = 10 * val + (*istream - '0');
        ++istream;
    } while (std::isdigit(*istream));

    token = val;
}


void Lexer::tokenize_terminal()
{
    Terminal term;
    char temp = *istream;
    ++istream;

    // Check for valid 2-char terminal symbol
    term = static_cast<Terminal>(encode(temp, *istream));
    if (terminals.contains(term)) {
        token = term;
        ++istream;
        return;
    }

    // Check for valid 1-char terminal symbol
    term = static_cast<Terminal>(encode(temp));
    if (terminals.contains(term)) {
        token = term;
        return;
    }

    // If it isn't a 1-char or 2-char terminal, then this is invalid!
    token = Invalid();
}


Lexer::Lexer(std::istream& in)
    :istream{in} {next();}


LexerException::LexerException(const std::string &msg) : message(msg) {}
const char* LexerException::what() const noexcept {
    return message.c_str();
}
