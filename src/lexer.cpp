#include "lexer.hpp"
/*
Deterministic Finite Automata of our Lexer
                [-------------------------------------START----------------------------------------]
         [a-z] /           |                   |                     |           |                 \ [0-9]
              /            | [;()*+-/.]        |[!]                  |[>]        | [<]              \ 
    [ IDENTIFIER ]   [ 1-char TERMINAL ]     ( no accept )   [ > TERMINAL ] [ < TERMINAL ]      [ CONSTANT ]
        | ^                                    | [=]                 | [=]      | [-=]               | ^
        |/ [a-z0-9]                            .-----> [ 2-char TERMINAL ] <----.                    |/ [0-9]
 */

void Token::print() {
  switch(type){
    case TokenType::TERMINAL:
      std::cout << "Terminal Token: " << static_cast<uint16_t>(std::get<Terminal>(payload)); 
      break;
    case TokenType::CONSTANT:
      std::cout << "Constant Token: " << std::get<int>(payload); 
      break;
    case TokenType::IDENTIFIER:
      std::cout << "Identifier Token: Index = " << std::get<int>(payload);
      break;
    case TokenType::INVALID:
      std::cout << "Invalid Token";
  }
  std::cout << std::endl;
}
void Lexer::next() {
  State state = State::START; std::string lexeme; char c; 
  do {
    input.get(c);
    switch(state){
      case State::START:
        if(std::isspace(c)) continue;
        else if(std::isalpha(c)) state = State::IDENTIFIER;
        else if(std::isdigit(c)) state = State::CONSTANT;
        else { term_tok(c); return; } // Either fails or returns terminal token.
        break;
      case State::IDENTIFIER:
        if(!isalnum(c)) { input.unget(); ident_tok(lexeme); return; }
        break;
      case State::CONSTANT:
        if(!std::isdigit(c)) { input.unget(); const_tok(lexeme); return; }
        break;
    }
    lexeme += c;
  }while(true);  
}

void Lexer::store(int index, int value) {
  identifier_values[index] = value;
}

int Lexer::load(int index) {
  return identifier_values[index];
}

void Lexer::ident_tok(const std::string& lexeme) {
  int index;
  if (!identifier_table.contains(lexeme)) {
    index = identifier_values.size();
    identifier_table[lexeme] = index;
    identifier_values.push_back(0);
  }
  else index = identifier_table[lexeme];
  token = Token{ TokenType::IDENTIFIER, identifier_table[lexeme] };
}

void Lexer::const_tok(const std::string& lexeme) {
  token = Token { TokenType::CONSTANT, std::stoi(lexeme) }; 
}

void Lexer::term_tok(const char& ch) {
  Terminal term;
  // Check for 2-char terminal
  char temp;
  input.get(temp);
  term = static_cast<Terminal>(encode(ch, temp));
  if(terminals.contains(term)) { 
    token = Token{ TokenType::TERMINAL, term };
    return;
  }
  input.unget(); 
  // Check for 1-char terminal
  term = static_cast<Terminal>(encode(ch));
  bool res = terminals.contains(term);
  if(res) token = Token{ TokenType::TERMINAL, term };
  else throw LexerException{ "Failed to tokenize given input. Cannot identify text as either an identifier, constant or terminal!" };
}

// int main() {
//   Lexer lexer(std::cin);
//   while (true) {
//     lexer.next();
//     lexer.token.print();
//   }
//   return 0;
// }
