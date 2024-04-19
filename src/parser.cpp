#include "parser.hpp"

template<typename... Args>
bool Parser::is_terminal_kind(const Token& token, Args... args) {
    if(token.type != TokenType::TERMINAL)
        return false;
    return ((std::get<Terminal>(token.payload) == args) || ...);
}

template<typename... Args>
bool Parser::is_identifier_index(const Token& token, Args... args) {
    if(token.type != TokenType::IDENTIFIER)
        return false;
    return ((std::get<int>(token.payload) == static_cast<int>(args)) || ...);
}

void Parser::variable_declaration() {
    if(is_identifier_index(lexer.token, Keyword::VAR)) {
        lexer.next();
        lexer.next(); // ident
        while(is_terminal_kind(lexer.token, Terminal::COMMA)) {
            lexer.next();
            lexer.next(); // ident
        }
        if(is_terminal_kind(lexer.token, Terminal::SEMICOLON))
            lexer.next();
    }
}

void Parser::statement_sequence() {
    if(is_terminal_kind(lexer.token, Terminal::LBRACE))
        lexer.next();
    while(!is_terminal_kind(lexer.token, Terminal::RBRACE)) {
        statement();
        if(is_terminal_kind(lexer.token, Terminal::SEMICOLON))
            lexer.next();
    }
}

void Parser::statement() {
    switch(lexer.token){


    }
}

int Parser::expression() {
    int result = term();
    while(is_terminal_kind(lexer.token, Terminal::PLUS, Terminal::MINUS)) { 
        if(is_terminal_kind(lexer.token, Terminal::PLUS)){
            lexer.next();
            result += factor();
        }      
        else {
            lexer.next();
            result -= factor();
        }
    }
    return result;
}

int Parser::term() {
    int result = factor();
    while(is_terminal_kind(lexer.token, Terminal::MUL, Terminal::DIV)) {
        if(is_terminal_kind(lexer.token, Terminal::MUL)) {
            lexer.next();
            result *= factor();
        }
        else {
            lexer.next();
            result /= factor();
        }
    }
    return result;
}

int Parser::factor() {
    int result = 0;
    if(lexer.token.type == TokenType::IDENTIFIER){
        result = lexer.load(std::get<int>(lexer.token.payload));
        lexer.next();
    } 
    else if(lexer.token.type == TokenType::CONSTANT) {
        result = std::get<int>(lexer.token.payload);
        lexer.next();
    }
    else if (is_terminal_kind(lexer.token, Terminal::LPAREN)) {
        lexer.next();
        result = expression();
        if(is_terminal_kind(lexer.token, Terminal::RPAREN))
            lexer.next();
    }
    return result;
}

Parser::Parser() : lexer(std::cin) { lexer.next(); }

void Parser::computation() {
    if(is_identifier_index(lexer.token, Keyword::MAIN)) {
        lexer.next();
        variable_declaration();
        // function_declaration();
        statement_sequence();
        if(is_terminal_kind(lexer.token, Terminal::PERIOD)) return;
    }
}

// int main() {
//   Parser p;
//   p.computation();
// }
