#include "tokenizer.hpp"
// Helpers
bool is_positive_number(const std::string &str) {
  return !str.empty() &&
         std::find_if(str.begin(), str.end(), [](unsigned char c) {
           return !std::isdigit(c);
         }) == str.end();
}

// Token classes
TerminalToken::TerminalToken(int k) : kind(k) {}
ConstantToken::ConstantToken(int val) : value(val) {}
IdentifierToken::IdentifierToken(int addr, int regno)
    : address(addr), reg_number(regno) {}

// TokenIterator
const std::map<std::string, int> TokenIterator::terminals{
    {";", SEMICOLON}, {"(", LPAREN}, {")", RPAREN}, {"*", MUL},    {"/", DIV},
    {"+", PLUS},      {"-", MINUS},  {".", PERIOD}, {"<-", ASSIGN}};
const std::regex TokenIterator::grammar{"\\b[a-z][a-z\\d]*\\b|\\b\\d+\\b|[;()*/+-.]|<-"};
TokenIterator::TokenIterator() : identifier_values(KEYWORD_COUNT, -1) {};

Token TokenIterator::next() {
  if (it == end)
    grab_chunk();
  std::string token_str = it->str();
  ++it;
  return construct_token(token_str);
}

void TokenIterator::store(int address, int value) {
  identifier_values[address] = value;
}

int TokenIterator::load(int address) {
  return identifier_values[address];
}

void TokenIterator::grab_chunk() {
  if (!std::getline(std::cin, current_line)) {
    throw std::runtime_error{"getline is done!"}; 
  }
  it = {current_line.begin(), current_line.end(), grammar};
}

Token TokenIterator::construct_token(const std::string &token_str) {
  // Terminals
  std::optional<Token> token = construct_terminal_token(token_str);
  if (token.has_value()) return token.value();

  // Constants
  token = construct_constant_token(token_str);
  if (token.has_value()) return token.value();

  // Identifiers
  token = construct_identifier_token(token_str);
  if (token.has_value()) return token.value();

  // Invalid token (should never happen)
  throw std::invalid_argument{"Invalid token: " + token_str};
}
std::optional<Token> 
TokenIterator::construct_terminal_token(const std::string &token_str) {
  auto it = terminals.find(token_str);
  if (it == terminals.end()) return std::nullopt;
  return TerminalToken{it->second};
}

std::optional<Token>
TokenIterator::construct_constant_token(const std::string &token_str) {
  try {
    int val = std::stoi(token_str);
    return ConstantToken{val};
  } catch (const std::exception &e) {
    return std::nullopt;
  }
}

std::optional<Token>
TokenIterator::construct_identifier_token(const std::string &token_str) {
  if (identifier_table.count(token_str) == 0) {
    int addr = identifier_values.size();
    identifier_table[token_str] = addr;
    identifier_values.push_back(0);
    return IdentifierToken{addr, -1};
  }
  return IdentifierToken{identifier_table[token_str], -1};
}

// TokenPrinter
void TokenPrinter::operator()(TerminalToken token) const {
  std::cout << "Terminal Token: " << token.kind << std::endl;
}

void TokenPrinter::operator()(ConstantToken token) const {
  std::cout << "Constant Token: " << token.value << std::endl;
}

void TokenPrinter::operator()(IdentifierToken token) const {
  std::cout << "Identifier Token: Address = " << token.address
            << ", RegNumber = " << token.reg_number << std::endl;
}

// int main() {
//   TokenIterator tokenIterator;
//   while (true) {
//     Token token = tokenIterator.next();
//     std::visit(TokenPrinter{}, token);
//   }
//   return 0;
// }
