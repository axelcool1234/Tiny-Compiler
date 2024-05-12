#include "token.hpp"

std::string to_string(const Token& t) {
    return std::visit([](auto&& arg) -> std::string
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Keyword>) {
            switch (arg) {
        #define KEYWORD(name, encoding) case Keyword::name: return #name;
            KEYWORD_LIST
        #undef KEYWORD
            default:
                return "UNKNOWN KEYWORD";
            }
        } else if constexpr (std::is_same_v<T, Terminal>) {
            switch (arg) {
        #define TERMINAL(name, encoding) case Terminal::name: return #name;
            TERMINAL_LIST
        #undef TERMINAL
            default:
                return "UNKNOWN TERMINAL";
            }
        } else if constexpr (std::is_same_v<T, ident_t>) {
            return std::to_string(arg);  
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, Invalid>) {
            return "INVALID TOKEN";
        } 
    }, t);
}
