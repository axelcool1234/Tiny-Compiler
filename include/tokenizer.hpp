#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <variant>

enum Terminal {
  SEMICOLON,
  LPAREN,
  RPAREN,
  MUL,
  DIV,
  PLUS,
  MINUS,
  PERIOD,
  ASSIGN,
};

enum Keyword {
  COMPUTATION,
  VAR,



  // Ensure this is at the end of the enum!!!!!!
  KEYWORD_COUNT
};

bool is_positive_number(const std::string &str);

class TerminalToken {
public:
    int kind;
    TerminalToken(int k);
};

class ConstantToken {
public:
  int value;
  ConstantToken(int val);
};

class IdentifierToken {
public:
  int address;
  int reg_number;
  IdentifierToken(int addr, int regno);
};

using Token = std::variant<TerminalToken, ConstantToken, IdentifierToken>;

class TokenIterator {
private:
  static const std::map<std::string, int> terminals;
  std::map<std::string, int> identifier_table{{"computation", COMPUTATION}, {"var", VAR}};
  std::vector<int> identifier_values;
  static const std::regex grammar;

  std::sregex_iterator it, end;
  std::string current_line;

  void grab_chunk();
  Token construct_token(const std::string &token_str);
  std::optional<Token> construct_terminal_token(const std::string &token_str);
  std::optional<Token> construct_constant_token(const std::string &token_str);
  std::optional<Token> construct_identifier_token(const std::string &token_str);

public:
  Token next();
  void store(int address, int value);
  int load(int address);
  TokenIterator();
};

class TokenPrinter {
public:
    void operator()(TerminalToken token) const;
    void operator()(ConstantToken token) const;
    void operator()(IdentifierToken token) const;
};

#endif // TOKENIZER_HPP
