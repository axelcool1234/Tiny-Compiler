#include "lexer.hpp"
#include "token.hpp"


/*
 *  Deterministic Finite Automata of our Lexer
 *              [-------------------------------------START----------------------------------------]
 *       [a-z] /           |                   |                     |           |                 \ [0-9]
 *            /            | [;()*+-/.]        |[!]                  |[>]        | [<]              \ 
 *  [ IDENTIFIER ]   [ 1-char TERMINAL ]     ( no accept )   [ > TERMINAL ] [ < TERMINAL ]      [ CONSTANT ]
 *      | ^                                    | [=]                 | [=]      | [-=]               | ^
 *      |/ [a-z0-9]                            .-----> [ 2-char TERMINAL ] <----.                    |/ [0-9] */


void Lexer::next() {
    // Advance past whitespace
    while (std::isspace(*istream)) { ++istream; }

    if (std::islower(*istream)) {
        tokenize_identifier();
    } else if (std::isdigit(*istream)) {
        tokenize_constant();
    } else {
        tokenize_terminal();
    }

    if (token.type == TokenType::INVALID) {
        throw LexerException("Failed to tokenize given input. Cannot identify text as either an identifier, constant or terminal!");
    }
}


void Lexer::tokenize_identifier() {
    std::string lexeme;

    while (std::islower(*istream) || std::isdigit(*istream)) {
        lexeme.push_back(*(istream++));
    }

    if (!identifier_table.contains(lexeme)) {
        identifier_table[lexeme] = ident_index;
        ident_index++;
    }

    token = Token {
        .type    = TokenType::IDENTIFIER,
        .payload = identifier_table.at(lexeme)
    };
};


void Lexer::tokenize_constant()
{
    int val{};

    // read in number, store in current token
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
    :istream{in} {}


LexerException::LexerException(const std::string &msg) : message(msg) {}
const char* LexerException::what() const noexcept {
    return message.c_str();
}


void Token::print() {
    switch (type) {
        case TokenType::TERMINAL:
            std::cout << "Terminal Token: " << static_cast<uint16_t>(std::get<Terminal>(payload)); 
            break;
        case TokenType::CONSTANT:
            std::cout << "Constant Token: " << std::get<int>(payload); 
            break;
        case TokenType::IDENTIFIER:
            std::cout << "Identifier Token: Index = " << std::get<int>(payload);
            break;
        default:
            std::cout << "Invalid Token";
    }
    std::cout << std::endl;
}
