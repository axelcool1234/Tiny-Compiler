#include "token.hpp"
#include <iostream>

void Token::print() {
    switch (type) {
        case TokenType::TERMINAL:
            std::cout << "Terminal Token: " << static_cast<uint16_t>(std::get<Terminal>(payload)); 
            break;
        case TokenType::CONSTANT:
            std::cout << "Constant Token: " << std::get<int>(payload); 
            break;
        case TokenType::USER_IDENTIFIER:
            std::cout << "User Identifier Token: Index = " << std::get<int>(payload);
            break;
        case TokenType::KEYWORD_IDENTIFIER:
            std::cout << "Keyword Identifier Token: Index = " << -std::get<int>(payload);
            break;
        default:
            std::cout << "Invalid Token";
    }
    std::cout << std::endl;
}
