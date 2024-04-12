#include "parser.hpp"

template<typename... Args>
bool Parser::is_terminal_kind(const Token& token, Args... args) {
  if(!std::holds_alternative<TerminalToken>(token))
    return false;
  return ((std::get<TerminalToken>(token).kind == args) || ...);
}

template<typename... Args>
bool Parser::is_identifier_address(const Token& token, Args... args) {
  if(!std::holds_alternative<IdentifierToken>(token))
    return false;
  return ((std::get<IdentifierToken>(token).address == args) || ...);
}

void Parser::variable() {
  while(is_identifier_address(token, VAR)) {
    token = it.next();
    int address = std::get<IdentifierToken>(token).address;
    token = it.next();
    if(is_terminal_kind(token, ASSIGN)) {
      token = it.next();
      it.store(address, expression());
    }
    if(is_terminal_kind(token, SEMICOLON))
      token = it.next();
  }
}

int Parser::expression() {
  int result = term();
  while(is_terminal_kind(token, PLUS, MINUS)) { 
    if(is_terminal_kind(token, PLUS)){
      token = it.next();
      result += factor();
    }      
    else {
      token = it.next();
      result -= factor();
    }
  }
  return result;
}

int Parser::term() {
  int result = factor();
  while(is_terminal_kind(token, MUL, DIV)) {
    if(is_terminal_kind(token, MUL)) {
      token = it.next();
      result *= factor();
    }
    else {
      token = it.next();
      result /= factor();
    }
  }
  return result;
}

int Parser::factor() {
  int result = 0;
  if(std::holds_alternative<IdentifierToken>(token)) {
    result = it.load(std::get<IdentifierToken>(token).address);
    token = it.next();
  } 
  else if(std::holds_alternative<ConstantToken>(token)) {
    result = std::get<ConstantToken>(token).value;
    token = it.next();
  }
  else if (is_terminal_kind(token, LPAREN)) {
    token = it.next();
    result = expression();
    if(is_terminal_kind(token, RPAREN))
      token = it.next();
  }
  return result;
}

Parser::Parser() : token{it.next()} {}

void Parser::computation() {
  if(is_identifier_address(token, COMPUTATION)) {
    token = it.next();
    variable();
    std::cout<< expression() << std::endl;
    while(is_terminal_kind(token, SEMICOLON)) {
      token = it.next();
      std::cout<< expression() << std::endl;
    }
    if(is_terminal_kind(token, PERIOD)) return;
  }
}

int main() {
  Parser p;
  p.computation();
}
