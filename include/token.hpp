#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>
#include <string>
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
    return (ch[1]) ? encode(ch[0], ch[1]) : ch[0];
}

#define TERMINAL_LIST \
    TERMINAL(SEMICOLON , ';' ) \
    TERMINAL(COMMA     , ',' ) \
    TERMINAL(LPAREN    , '(' ) \
    TERMINAL(RPAREN    , ')' ) \
    TERMINAL(LBRACE    , '{' ) \
    TERMINAL(RBRACE    , '}' ) \
    TERMINAL(MUL       , '*' ) \
    TERMINAL(DIV       , '/' ) \
    TERMINAL(PLUS      , '+' ) \
    TERMINAL(MINUS     , '-' ) \
    TERMINAL(PERIOD    , '.' ) \
    TERMINAL(ASSIGN    , "<-") \
    TERMINAL(EQ        , "==") \
    TERMINAL(NEQ       , "!=") \
    TERMINAL(LT        , '<' ) \
    TERMINAL(LE        , "<=") \
    TERMINAL(GT        , '>' ) \
    TERMINAL(GE        , ">=") \

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
    KEYWORD(VAR         , var       ) \
    KEYWORD(LET         , let       ) \
    KEYWORD(CALL        , call      ) \
    KEYWORD(IF          , if        ) \
    KEYWORD(THEN        , then      ) \
    KEYWORD(ELSE        , else      ) \
    KEYWORD(FI          , fi        ) \
    KEYWORD(WHILE       , while     ) \
    KEYWORD(DO          , do        ) \
    KEYWORD(OD          , od        ) \
    KEYWORD(RETURN      , return    ) \
    KEYWORD(VOID        , void      ) \
    KEYWORD(FUNCTION    , function  ) \
    KEYWORD(MAIN        , main      ) \

enum class Keyword {
    DUMMY,
#define KEYWORD(name, encoding) name,
    KEYWORD_LIST
#undef KEYWORD
    KEYWORD_COUNT
};

enum class TokenType {
    INVALID,            /* Initial state of a token if default initialized. */
    TERMINAL,           /* Refers to terminal symbols of the language. i.e. '<' or '+'. */
    CONSTANT,           /* Refers to constant values, only integers are supported in Tiny. */
    USER_IDENTIFIER,    /* Refers to identifiers for variables and functions. */
    KEYWORD_IDENTIFIER, /* Refers to identifiers for reserved keywords. */
};

using ident_t = size_t;
struct Token {
    TokenType type;
    std::variant<Terminal, Keyword, ident_t, int> payload;

    /*
     * Prints the token's information based off what type it is.
     */
    void print();
};

#endif // TOKEN_HPP
