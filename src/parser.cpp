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
    return ((std::get<int>(token.payload) == -static_cast<int>(args)) || ...);
}

Opcode Parser::terminal_to_opcode(const Terminal& terminal) {
    switch(terminal) {
        case Terminal::EQ:
            return Opcode::BNE;
        case Terminal::NEQ:
            return Opcode::BEQ;
        case Terminal::LT:
            return Opcode::BGE;
        case Terminal::LE:
            return Opcode::BGT;
        case Terminal::GT:
            return Opcode::BLE;
        case Terminal::GE:
            return Opcode::BLT;
        default:
            throw ParserException("Invalid terminal to opcode conversion");
    }
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

void Parser::main_statement_sequence(bb_t& curr_block) {
    if(is_terminal_kind(lexer.token, Terminal::LBRACE))
        lexer.next();
    while(!is_terminal_kind(lexer.token, Terminal::RBRACE)) {
        statement(curr_block);
        if(is_terminal_kind(lexer.token, Terminal::SEMICOLON))
            lexer.next();
    }
}

void Parser::then_statement_sequence(bb_t& curr_block) {
    while(!is_identifier_index(lexer.token, Keyword::ELSE, Keyword::FI)) {
        statement(curr_block);
        if(is_terminal_kind(lexer.token, Terminal::SEMICOLON))
            lexer.next();
    }
}

void Parser::else_statement_sequence(bb_t& curr_block) {
    while(!is_identifier_index(lexer.token, Keyword::FI)) {
        statement(curr_block);
        if(is_terminal_kind(lexer.token, Terminal::SEMICOLON))
            lexer.next();
    }
}

void Parser::statement(bb_t& curr_block) {
    if(is_identifier_index(lexer.token, Keyword::LET)) {
        lexer.next();
        let_statement(curr_block);
    }
    else if(is_identifier_index(lexer.token, Keyword::CALL)) {
        lexer.next();
        func_statement(curr_block);
    }
    else if(is_identifier_index(lexer.token, Keyword::IF)) {
        lexer.next();
        if_statement(curr_block);
    }
    else if(is_identifier_index(lexer.token, Keyword::WHILE)) {
        lexer.next();
        while_statement(curr_block);
    }
    else if(is_identifier_index(lexer.token, Keyword::RETURN)) {
        lexer.next();
        return_statement(curr_block);
    }
    else throw ParserException("Parser error in statement function.");
}

void Parser::let_statement(const bb_t& curr_block) {
    ident_t ident = std::get<int>(lexer.token.payload);
    lexer.next();
    if(is_terminal_kind(lexer.token, Terminal::ASSIGN)){
        lexer.next();
        ir.change_ident_value(curr_block, ident, expression(curr_block));
    }
}

void Parser::func_statement(bb_t& curr_block) {

}

void Parser::if_statement(bb_t& curr_block) {
    relation(curr_block);
    bb_t then_block = ir.new_block(curr_block, FALLTHROUGH);
    if(is_identifier_index(lexer.token, Keyword::THEN)) {
        lexer.next();
        then_statement_sequence(then_block);
    }
    bb_t else_block = ir.new_block(curr_block, BRANCH);
    const bb_t og_else_block = else_block;
    if(is_identifier_index(lexer.token, Keyword::ELSE)) {
        lexer.next();        
        else_statement_sequence(else_block);
    }
    if(is_identifier_index(lexer.token, Keyword::FI)) {
        lexer.next();
    }
    bb_t og_curr_block = curr_block;
    curr_block = ir.new_block(then_block, else_block);
    ir.set_branch_cond(then_block, Opcode::BRA, ir.first_instruction(curr_block));
    ir.set_branch_location(og_curr_block, ir.first_instruction(og_else_block));
}

void Parser::while_statement(bb_t& curr_block) {

}

void Parser::return_statement(bb_t& curr_block) {

}

void Parser::relation(const bb_t& curr_block) {
    instruct_t x = expression(curr_block);
    Terminal rel_op = std::get<Terminal>(lexer.token.payload);
    lexer.next();
    instruct_t y = expression(curr_block);
    instruct_t cmp = ir.add_instruction(curr_block, Opcode::CMP, x, y);
    ir.set_branch_cond(curr_block, terminal_to_opcode(rel_op), cmp);    
}

instruct_t Parser::expression(const bb_t& curr_block) {
    instruct_t result = term(curr_block);
    while(is_terminal_kind(lexer.token, Terminal::PLUS, Terminal::MINUS)) { 
        if(is_terminal_kind(lexer.token, Terminal::PLUS)){
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::ADD, result, term(curr_block));
        }      
        else {
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::SUB, result, term(curr_block));
        }
    }
    return result;
}

instruct_t Parser::term(const bb_t& curr_block) {
    instruct_t result = factor(curr_block);
    while(is_terminal_kind(lexer.token, Terminal::MUL, Terminal::DIV)) {
        if(is_terminal_kind(lexer.token, Terminal::MUL)) {
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::MUL, result, factor(curr_block));
        }
        else {
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::DIV, result, factor(curr_block));
        }
    }
    return result;
}

instruct_t Parser::factor(const bb_t& curr_block) {
    instruct_t result;
    if(lexer.token.type == TokenType::IDENTIFIER){
        result = ir.get_ident_value(curr_block, std::get<int>(lexer.token.payload));
        lexer.next();
        return result;
    } 
    else if(lexer.token.type == TokenType::CONSTANT) {
        result = ir.add_instruction(const_block, Opcode::CONST, std::get<int>(lexer.token.payload));        
        lexer.next();
        return result;
    }
    else if (is_terminal_kind(lexer.token, Terminal::LPAREN)) {
        lexer.next();
        result = expression(curr_block);
        if(is_terminal_kind(lexer.token, Terminal::RPAREN))
            lexer.next();
        return result;
    }
    throw ParserException("Parser error in factor function.");
}

Parser::Parser() : lexer(std::cin) { lexer.next(); }

void Parser::computation() {
    if(is_identifier_index(lexer.token, Keyword::MAIN)) {
        lexer.next();
        variable_declaration();
        // function_declaration();
        ir.establish_const_block(lexer.ident_index);
        bb_t curr_block = ir.new_block(const_block);
        main_statement_sequence(curr_block);
        std::cout << ir.to_dotlang();
        if(is_terminal_kind(lexer.token, Terminal::PERIOD)) return;
    }
}

int main() {
  Parser p;
  p.computation();
}
