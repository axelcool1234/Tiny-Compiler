#ifndef LEXER_HPP
#define LEXER_HPP

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <istream>
#include <map>
#include <stdexcept>
#include <unordered_set>
#include <variant>
#include <vector>

inline constexpr std::uint16_t encode(char ch) {
  return ch;
}
inline constexpr std::uint16_t encode(char ch1, char ch2) {
  return ch1 * 256 + ch2;
}
inline constexpr std::uint16_t encode(const char ch[2]) {
  if (ch[1] != '\0') return ch[0] * 256 + ch[1];
  else return ch[0];
}

#define TERMINAL_LIST \
    TERMINAL(SEMICOLON, ';') \
    TERMINAL(LPAREN, '(') \
    TERMINAL(RPAREN, ')') \
    TERMINAL(MUL, '*') \
    TERMINAL(DIV, '/') \
    TERMINAL(PLUS, '+') \
    TERMINAL(MINUS, '-') \
    TERMINAL(PERIOD, '.') \
    TERMINAL(ASSIGN, "<-") \
    TERMINAL(EQ, "==") \
    TERMINAL(NEQ, "!=") \
    TERMINAL(LT, '<') \
    TERMINAL(LE, "<=") \
    TERMINAL(GT, '>') \
    TERMINAL(GE, ">=")

enum class Terminal : uint16_t {
#define TERMINAL(name, encoding) name = encode(encoding),
    TERMINAL_LIST
#undef TERMINAL
};

static const std::unordered_set<Terminal> terminals{
#define TERMINAL(name, encoding) Terminal::name,
    TERMINAL_LIST
#undef TERMINAL
};

#define KEYWORD_LIST \
    KEYWORD(COMPUTATION, computation) \
    KEYWORD(VAR, var) \
    KEYWORD(LET, let) \
    KEYWORD(CALL, call) \
    KEYWORD(IF, if) \
    KEYWORD(THEN, then) \
    KEYWORD(ELSE, else) \
    KEYWORD(FI, fi) \
    KEYWORD(WHILE, while) \
    KEYWORD(DO, do) \
    KEYWORD(OD, od) \
    KEYWORD(RETURN, return) \
    KEYWORD(VOID, void) \
    KEYWORD(FUNCTION, function) \
    KEYWORD(MAIN, main)

enum class Keyword {
#define KEYWORD(name, encoding) name,
    KEYWORD_LIST
#undef KEYWORD
    KEYWORD_COUNT
};

enum class TokenType {
  INVALID, // Initial state of a token if default initialized.
  TERMINAL,
  CONSTANT,
  IDENTIFIER,
};

class Token {
public:
  std::variant<Terminal, int> payload;
  TokenType type;
  void print();
  Token(TokenType ty, std::variant<Terminal, int> pl) : payload(pl), type(ty) {}
  Token() = default;
};

class Lexer {
private:
  enum class State {
    START,
    IDENTIFIER,
    CONSTANT,
  };
  std::map<std::string, int> identifier_table{
  #define KEYWORD(name, encoding) {#encoding, static_cast<int>(Keyword::name)},
      KEYWORD_LIST
  #undef KEYWORD
  };
  std::vector<int> identifier_values;
  std::istream& input;
  void unget(const char& ch);
  void ident_tok(const std::string& lexeme);
  void const_tok(const std::string& lexeme);
  void term_tok(const char& ch);
public:
  Token token;
  Lexer(std::istream& in) : identifier_values(static_cast<int>(Keyword::KEYWORD_COUNT), -1), input(in) {}
  void next();
  void store(int index, int value);
  int load(int index);
};

class LexerException : public std::exception {
private:
  std::string message;
public:
  LexerException(const std::string& msg) : message(msg) {}
  const char* what() const noexcept override {
    return message.c_str();
  }
};

#endif // LEXER_HPP
