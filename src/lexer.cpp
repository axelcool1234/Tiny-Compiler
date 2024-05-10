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


void Lexer::next() {
    // Always skip whitespace
    while (std::isspace(*istream)) { ++istream; }

    // If we are at EOF, then always return invalid tokens
    if (istream == std::istreambuf_iterator<char>{}) {
        token.type = TokenType::INVALID;
        return;
    }

    if (std::islower(*istream)) { // lowercase alphas (for the first char) belong to identifier tokens.
        tokenize_identifier();
    } else if (std::isdigit(*istream)) { // digits (for the first char) belong to constant tokens.
        tokenize_constant();
    } else { // symbols are for terminals.
        tokenize_terminal();
    }

    if (token.type == TokenType::INVALID) {
        throw LexerException("Failed to tokenize given input. Cannot identify text as either an identifier, constant or terminal!");
    }
}


void Lexer::tokenize_identifier() {
    std::string lexeme;

    // identifiers can have lowercase alphas or digits (just lowercase alphas for the first char)
    while (std::islower(*istream) || std::isdigit(*istream)) {
        lexeme.push_back(*(istream++));
    }

    // Check if this identifier has been seen before
    if (!identifier_table.contains(lexeme)) {
        identifier_table[lexeme] = ident_index;
        ident_index++;
    }

    const int& ident_value = identifier_table.at(lexeme);     
    if(ident_value >= 0) {
        token = Token {
            .type    = TokenType::USER_IDENTIFIER,
            .payload = static_cast<ident_t>(ident_value)
        };
    } else {
        token = Token {
            .type    = TokenType::KEYWORD_IDENTIFIER,
            .payload = static_cast<Keyword>(-ident_value)
        };
    }
};


void Lexer::tokenize_constant()
{
    int val{};

    // Read until a non-digit is found (builds an integer for the const token)
    do {
        val = 10 * val + (*istream - '0');
        ++istream;
    } while (std::isdigit(*istream));

    token = Token {
        .type    = TokenType::CONSTANT,
        .payload = val
    }; 
}


void Lexer::tokenize_terminal()
{
    Terminal term;
    char temp = *istream;
    ++istream;

    // Check for valid 2-char terminal symbol
    term = static_cast<Terminal>(encode(temp, *istream));
    if (terminals.contains(term)) {
        token = Token{
            .type    = TokenType::TERMINAL,
            .payload = term
        };
        ++istream;
        return;
    }

    // Check for valid 1-char terminal symbol
    term = static_cast<Terminal>(encode(temp));
    if (terminals.contains(term)) {
        token = Token{
            .type    = TokenType::TERMINAL,
            .payload = term
        };
        return;
    }

    // If it isn't a 1-char or 2-char terminal, then this is invalid!
    token.type = TokenType::INVALID;
}


Lexer::Lexer(std::istream& in)
    :istream{in} {next();}


LexerException::LexerException(const std::string &msg) : message(msg) {}
const char* LexerException::what() const noexcept {
    return message.c_str();
}


std::string Lexer::to_string(Keyword k) {
    switch (k) {
        case Keyword::VAR:
            return {"VAR"};
        case Keyword::LET:
            return {"LET"};
        case Keyword::CALL:
            return {"CALL"};
        case Keyword::IF:
            return {"IF"};
        case Keyword::THEN:
            return {"THEN"};
        case Keyword::ELSE:
            return {"ELSE"};
        case Keyword::FI:
            return {"FI"};
        case Keyword::WHILE:
            return {"WHILE"};
        case Keyword::DO:
            return {"DO"};
        case Keyword::OD:
            return {"OD"};
        case Keyword::RETURN:
            return {"RETURN"};
        case Keyword::VOID:
            return {"VOID"};
        case Keyword::FUNCTION:
            return {"FUNCTION"};
        case Keyword::MAIN:
            return {"MAIN"};
        default:
            return {""};
    }
}


std::string Lexer::to_string(Terminal t) {
    switch (t) {
        case Terminal::SEMICOLON:
            return {"VAR"};
        case Terminal::COMMA:
            return {"LET"};
        case Terminal::LPAREN:
            return {"CALL"};
        case Terminal::RPAREN:
            return {"IF"};
        case Terminal::LBRACE:
            return {"THEN"};
        case Terminal::RBRACE:
            return {"ELSE"};
        case Terminal::MUL:
            return {"FI"};
        case Terminal::DIV:
            return {"WHILE"};
        case Terminal::PLUS:
            return {"DO"};
        case Terminal::MINUS:
            return {"OD"};
        case Terminal::PERIOD:
            return {"RETURN"};
        case Terminal::ASSIGN:
            return {"VOID"};
        case Terminal::EQ:
            return {"FUNCTION"};
        case Terminal::NEQ:
            return {"MAIN"};
        case Terminal::LT:
            return {"MAIN"};
        case Terminal::LE:
            return {"MAIN"};
        case Terminal::GT:
            return {"MAIN"};
        case Terminal::GE:
            return {"MAIN"};
        default:
            return {};
    }
}
