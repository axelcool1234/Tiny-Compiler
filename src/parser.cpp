#include "parser.hpp"
#include "token.hpp"
#include <format>

/* API */
void Parser::parse() {
    computation();
}

Parser::Parser() : lexer(std::cin) {}

void Parser::print() {
    std::cout << ir.to_dotlang();
}

/* Parsing main function of Tiny program */
void Parser::computation() {
    /* "main" [ variable_declaration ] { function_declaration } "{" statSequence "}" "." */
    match(Keyword::MAIN);
    bb_t curr_block = variable_declaration();
    // function_declaration();
    match(Terminal::LBRACE);
    statement_sequence(curr_block, Terminal::RBRACE);
    match(Terminal::RBRACE);
    match(Terminal::PERIOD);
}

/* Declarations */
bb_t Parser::variable_declaration() {
    /* "var" ident { "," ident } ";" */
    if(token_is(lexer.token, Keyword::VAR)){
        lexer.next();
        match<ident_t>();
        while(token_is(lexer.token, Terminal::COMMA)) {
            lexer.next();
            match<ident_t>();
        }
        match(Terminal::SEMICOLON);
    }
    ir.establish_const_block(lexer.ident_index);
    return ir.new_block(const_block);
}

void function_declaration() {
    
}

/* Statements  */
template<typename... Args>
void Parser::statement_sequence(bb_t& curr_block, Args... args) {
    bool prev_ignore = ir.ignore;
    bb_t dummy = -1;
    while((!token_is(lexer.token, args) && ...)) {
        if(prev_ignore || ir.ignore) {
            statement(dummy);
        } else {
            ir.ignore = statement(curr_block);
        }
        // TODO: Figure out a way to enforce semicolons for all but the last statement.
        if(token_is(lexer.token, Terminal::SEMICOLON))
            lexer.next();
    } 
    ir.ignore = prev_ignore;
}

bool Parser::statement(bb_t& curr_block) {
    switch(match_return<Keyword>()) {
        case Keyword::LET:
            let_statement(curr_block);
            return false;
        case Keyword::CALL:
            void_function_statement(curr_block);
            return false;
        case Keyword::IF:
            return if_statement(curr_block);
        case Keyword::WHILE:
            while_statement(curr_block);
            return false;
        case Keyword::RETURN:
            return_statement(curr_block);
            return true;
        default:
            throw ParserException(std::format("Invalid reserved keyword in statement! Received {}", to_string(lexer.token)));
    }    
}

void Parser::let_statement(const bb_t& curr_block) {
    /* ident "<-" expression */
    ident_t ident = match_return<ident_t>();
    match(Terminal::ASSIGN);
    ir.change_ident_value(curr_block, ident, expression(curr_block));
}

void Parser::nonvoid_function_statement(const bb_t& curr_block) {

}

void Parser::void_function_statement(const bb_t& curr_block) {

}

void Parser::function_statement(const bb_t& curr_block) {

}

bool Parser::if_statement(bb_t& curr_block) {
    /* relation "then" statement_sequence [ "else" statement_sequence ] "fi" */
    relation(curr_block);
    match(Keyword::THEN);
    bb_t then_block = ir.new_block(curr_block, FALLTHROUGH);
    statement_sequence(then_block, Keyword::ELSE, Keyword::FI);

    // "else"
    bb_t else_block = ir.new_block(curr_block, BRANCH);
    const bb_t og_else_block = else_block;
    if(token_is(lexer.token, Keyword::ELSE)) {
        lexer.next();        
        statement_sequence(else_block, Keyword::FI);
    }
    match(Keyword::FI);

    // Set branch location for dominator block
    ir.set_branch_location(curr_block, ir.first_instruction(og_else_block));

    // If both blocks return, no need to continue.
    if(ir.will_return(then_block) && ir.will_return(else_block)) {
         ir.set_return(curr_block);
         return true;
    }

    // If one of the blocks returns, no need to join.
    if(ir.will_return(then_block)) {
         curr_block = else_block;
         return false;
    } else if (ir.will_return(else_block)) {
        curr_block = then_block;
        return false;
    }

    // Create JOIN block if both blocks do not return.
    join(curr_block, then_block, else_block);
    return false;
}

void Parser::join(bb_t& curr_block, const bb_t& then_block, const bb_t& else_block) {
    curr_block = ir.new_block(then_block, else_block, curr_block);
    ir.set_branch_cond(then_block, Opcode::BRA, ir.first_instruction(curr_block));
}

void Parser::while_statement(bb_t& curr_block) {
    /* "while" relation "do" statement_sequence "od" */
    curr_block = ir.new_block(curr_block);

    // Relation
    relation(curr_block);

    bb_t while_block = ir.new_block(curr_block, FALLTHROUGH);
    const bb_t og_while_block = while_block;

    // "do" statement_sequence "od"
    match(Keyword::DO);
    statement_sequence(while_block, Keyword::OD);
    match(Keyword::OD);

    // Create BRANCH block
    branch(curr_block, while_block);
}

void Parser::branch(bb_t& curr_block, const bb_t& while_block) {
    bb_t og_curr_block = curr_block;
    ir.generate_phi(og_curr_block, while_block);
    ir.set_branch_cond(while_block, Opcode::BRA, ir.first_instruction(og_curr_block));
    curr_block = ir.new_block(curr_block, BRANCH);
    ir.set_branch_location(og_curr_block, ir.first_instruction(curr_block));
}

void Parser::return_statement(bb_t& curr_block) {
    // [ expression ]
    // Check for a potential expression after "return"
    if(token_is(lexer.token, Keyword::CALL) ||
       token_is(lexer.token, Terminal::LPAREN, Terminal::RPAREN) ||
       token_is<int>(lexer.token) ||
       token_is<ident_t>(lexer.token)) {
        instruct_t expr = expression(curr_block);
        ir.set_branch_cond(curr_block, Opcode::RET, expr);
        return;
    }
    ir.set_branch_cond(curr_block, Opcode::RET, -1);
}

/* Relations */
void Parser::relation(const bb_t& curr_block) {
    /* expression1 rel_op expression2 */
    instruct_t x = expression(curr_block);
    Terminal rel_op = match_return<Terminal>();
    instruct_t y = expression(curr_block);

    // Set up CMP instruction and branch instruction.
    instruct_t cmp = ir.add_instruction(curr_block, Opcode::CMP, x, y);
    ir.set_branch_cond(curr_block, terminal_to_opcode(rel_op), cmp);    
}

/* Base Parsing */
instruct_t Parser::expression(const bb_t& curr_block) {
    instruct_t result = term(curr_block);
    while(token_is(lexer.token, Terminal::PLUS, Terminal::MINUS)) {
        if(token_is(lexer.token, Terminal::PLUS)) {
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::ADD, result, term(curr_block));
        } else {
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::SUB, result, term(curr_block));
        }
    }
    return result;
}

instruct_t Parser::term(const bb_t& curr_block) {
    instruct_t result = factor(curr_block);
    while(token_is(lexer.token, Terminal::MUL, Terminal::DIV)) {
        if(token_is(lexer.token, Terminal::MUL)) {
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::MUL, result, factor(curr_block));
        } else {
            lexer.next();
            result = ir.add_instruction(curr_block, Opcode::DIV, result, factor(curr_block));
        }
    }
    return result;
}

instruct_t Parser::factor(const bb_t& curr_block) {
    instruct_t result;
    if(token_is<ident_t>(lexer.token)) {
        result = ir.get_ident_value(curr_block, match_return<ident_t>());
        return result;
    } else if (token_is<int>(lexer.token)) { 
        result = ir.add_instruction(const_block, Opcode::CONST, match_return<int>());        
        return result;
    } else if (token_is(lexer.token, Terminal::LPAREN)) {
        lexer.next();
        result = expression(curr_block);
        if(token_is(lexer.token, Terminal::RPAREN))
            lexer.next();
        return result;
    } else if (token_is(lexer.token, Keyword::CALL)) {
        lexer.next();
        nonvoid_function_statement(curr_block);
    }
    throw ParserException("Expected user identifier, constant, function call, or '(' in factor.");
}

/* Helpers */
template<typename T, typename... Args>
bool Parser::token_is(const Token& token, const T& expected, const Args&... args) {
    return (std::holds_alternative<T>(token)) && 
           ((std::get<T>(token) == expected) ||
           ((std::get<T>(token) == args) || ...));
}

template<typename T>
bool Parser::token_is(const Token& token) {
    return std::holds_alternative<T>(token);
}

template<typename T>
void Parser::assert_type() {
    if (!token_is<T>(lexer.token))
        throw ParserException(std::format("Expected type {}, received type {} instead .", typeid(T).name(), to_string(lexer.token))); 
}

template<typename T>
void Parser::match(const T& expected) {
    if(!token_is(lexer.token, expected)) 
        throw ParserException(std::format("Expected {}, received {} instead.", to_string(expected), to_string(lexer.token))); 
    lexer.next();
}

template<typename T>
void Parser::match() {
    assert_type<T>();
    lexer.next();
}

template<typename T>
T Parser::match_return() {
    assert_type<T>();
    T obj = std::get<T>(lexer.token);
    lexer.next();
    return obj;
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
            throw ParserException("Invalid terminal to opcode conversion!");
    }
}
