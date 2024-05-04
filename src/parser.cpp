#include "parser.hpp"

template<typename... Args>
bool Parser::is_terminal(const Token& token, Args... args) {
    if(token.type != TokenType::TERMINAL)
        return false;
    return ((std::get<Terminal>(token.payload) == args) || ...);
}
bool Parser::is_terminal(const Token& token) {
    return token.type == TokenType::TERMINAL;
}

template<typename... Args>
bool Parser::is_keyword_identifier(const Token& token, Args... args) {
    if(token.type != TokenType::KEYWORD_IDENTIFIER)
        return false;
    return ((std::get<Keyword>(token.payload) == args) || ...);
}
bool Parser::is_keyword_identifier(const Token& token) {
    return token.type != TokenType::KEYWORD_IDENTIFIER;
}


bool Parser::is_user_identifier(const Token& token) {
    return token.type == TokenType::USER_IDENTIFIER;
}

template<typename... Args>
void Parser::keyword_sequence_helper(bb_t& curr_block, Args... args) {
    while((!is_keyword_identifier(lexer.token, args) && ...)) {
        statement_handler(curr_block);
    } 
}

template<typename... Args>
void Parser::terminal_sequence_helper(bb_t& curr_block, Args... args) {
    while((!is_terminal(lexer.token, args) && ...)) {
        statement_handler(curr_block);
    } 
}

void Parser::statement_handler(bb_t& curr_block) {
    statement(curr_block);
    // TODO: Figure out a way to enforce semicolons for all but the last statement.
    if(is_terminal(lexer.token, Terminal::SEMICOLON))
        lexer.next();
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

void Parser::check_and_next(Keyword k, const std::string& msg) {
    if(!is_keyword_identifier(lexer.token, k)) 
        throw ParserException(msg);
    lexer.next();
}

void Parser::check_and_next(Terminal t, const std::string& msg) {
    if(!is_terminal(lexer.token, t))
        throw ParserException(msg);
    lexer.next();
}

void Parser::check_and_next(const std::string& msg) {
    if(!is_user_identifier(lexer.token))
        throw ParserException(msg);
    lexer.next();
}

template<typename T>
T Parser::check_and_next_return(const std::string& msg) {
    if (std::is_same<T, ident_t>::value) {
        if(!is_user_identifier(lexer.token))
            throw ParserException(msg);
    } 
    else if (std::is_same<T, Terminal>::value) {
        if(!is_terminal(lexer.token))
            throw ParserException(msg);
    }
    T obj = std::get<T>(lexer.token.payload);
    lexer.next();
    return obj;
}

Parser::Parser() : lexer(std::cin) { lexer.next(); }

void Parser::parse() {
    computation();
}

void Parser::print() {
    std::cout << ir.to_dotlang();
}

void Parser::computation() {
    /* "main" [ variable_declaration ] { function_declaration } "{" statSequence "}" "." */
    main();
    bb_t curr_block = variable_declaration();
    // function_declaration();
    lbrace();
    main_statement_sequence(curr_block);
    rbrace();
    period();
}

void Parser::main() {
    check_and_next(Keyword::MAIN, "Expected 'main'!");
}

void Parser::lbrace() {
    check_and_next(Terminal::LBRACE, "Expected '{' for main!"); 
}

void Parser::rbrace() {
    check_and_next(Terminal::RBRACE, "Expected '}' for main!"); 
}

void Parser::period() {
    if(!is_terminal(lexer.token, Terminal::PERIOD))
        throw ParserException("Expected '.' for main!");
}

/* Declarations */
bb_t Parser::variable_declaration() {
    /* "var" ident { "," ident } ";" */
    var();
    variable_identifier();
    while(is_terminal(lexer.token, Terminal::COMMA)) {
        lexer.next();
        variable_identifier();
    }
    semicolon();
    ir.establish_const_block(lexer.ident_index);
    return ir.new_block(const_block);
}

void Parser::var() {
    check_and_next(Keyword::VAR,"Expected 'var' in variable declaration!");
}

void Parser::variable_identifier() {
    check_and_next("Expected user identifier in variable declaration!");
}

void Parser::semicolon() {
    check_and_next(Terminal::SEMICOLON,"Expected ';' in variable declaration!");
}

void function_declaration() {
    
}

/* Statement Sequences */
void Parser::main_statement_sequence(bb_t& curr_block) {
    terminal_sequence_helper(curr_block, Terminal::RBRACE);
}

void Parser::then_statement_sequence(bb_t& then_block) {
    keyword_sequence_helper(then_block, Keyword::ELSE, Keyword::FI);
}

void Parser::else_statement_sequence(bb_t& else_block) {
    keyword_sequence_helper(else_block, Keyword::FI);
}

void Parser::while_statement_sequence(bb_t& while_block) {
    keyword_sequence_helper(while_block, Keyword::OD);
}

/* Statements */
void Parser::statement(bb_t& curr_block) {
    if(lexer.token.type != TokenType::KEYWORD_IDENTIFIER)
        throw ParserException("Expected reserved keyword in statement!");
    switch(std::get<Keyword>(lexer.token.payload)) {
        case Keyword::LET:
            lexer.next();
            let_statement(curr_block);
            break;
        case Keyword::CALL:
            lexer.next();
            func_statement(curr_block);
            break;
        case Keyword::IF:
            lexer.next();
            if_statement(curr_block);
            break;
        case Keyword::WHILE:
            lexer.next();
            while_statement(curr_block);
            break;
        case Keyword::RETURN:
            lexer.next();
            return_statement(curr_block);
            break;
        default:
            throw ParserException("Invalid reserved keyword in statement!");
    }    
}

void Parser::let_statement(const bb_t& curr_block) {
    /* ident "<-" expression */
    ident_t ident = let_ident();
    let_assign();
    ir.change_ident_value(curr_block, ident, expression(curr_block));
}

ident_t Parser::let_ident() {
    return check_and_next_return<ident_t>("Expected user identifier in let statement!"); 
}

void Parser::let_assign() {
    check_and_next(Terminal::ASSIGN,"Expected assignment terminal '<-' in let statement!");
}

void Parser::func_statement(bb_t& curr_block) {

}

void Parser::if_statement(bb_t& curr_block) {
    /* relation "then" statement_sequence [ "else" statement_sequence ] "fi" */
    relation(curr_block);
    then();
    bb_t then_block = ir.new_block(curr_block, FALLTHROUGH);
    then_statement_sequence(then_block);

    // "else"
    bb_t else_block = ir.new_block(curr_block, BRANCH);
    const bb_t og_else_block = else_block;
    if(is_keyword_identifier(lexer.token, Keyword::ELSE)) {
        lexer.next();        
        else_statement_sequence(else_block);
    }
    fi();

    // Create JOIN block
    join(curr_block, then_block, else_block, og_else_block);
}

void Parser::then() {
    check_and_next(Keyword::THEN,"Expected 'then' in if statement!");
}

void Parser::fi() {
    check_and_next(Keyword::FI,"Expected 'fi' in if statement!");
}

void Parser::join(bb_t& curr_block, const bb_t& then_block, const bb_t& else_block, const bb_t& og_else_block) {
    bb_t og_curr_block = curr_block;
    curr_block = ir.new_block(then_block, else_block, og_curr_block);
    ir.set_branch_cond(then_block, Opcode::BRA, ir.first_instruction(curr_block));
    ir.set_branch_location(og_curr_block, ir.first_instruction(og_else_block));
}

void Parser::while_statement(bb_t& curr_block) {
    /* "while" relation "do" statement_sequence "od" */
    curr_block = ir.new_block(curr_block);

    // Relation
    relation(curr_block);

    bb_t while_block = ir.new_block(curr_block, FALLTHROUGH);
    const bb_t og_while_block = while_block;

    // "do" statement_sequence "od"
    while_do();
    while_statement_sequence(while_block);
    while_od();

    // Create BRANCH block
    branch(curr_block, while_block);
}

void Parser::while_do() {
    check_and_next(Keyword::DO,"Expected 'do' in while statement!");
}

void Parser::while_od() {
    check_and_next(Keyword::OD,"Expected 'od' in while statement!");
}

void Parser::branch(bb_t& curr_block, const bb_t& while_block) {
    bb_t og_curr_block = curr_block;
    ir.generate_phi(og_curr_block, while_block);
    ir.set_branch_cond(while_block, Opcode::BRA, ir.first_instruction(og_curr_block));
    curr_block = ir.new_block(curr_block, BRANCH);
    ir.set_branch_location(og_curr_block, ir.first_instruction(curr_block));
}

void Parser::return_statement(bb_t& curr_block) {

}

/* Relations */
void Parser::relation(const bb_t& curr_block) {
    /* expression1 rel_op expression2 */
    instruct_t x = expression(curr_block);
    Terminal rel_op = relation_op();
    instruct_t y = expression(curr_block);

    // Set up CMP instruction and branch instruction.
    instruct_t cmp = ir.add_instruction(curr_block, Opcode::CMP, x, y);
    ir.set_branch_cond(curr_block, terminal_to_opcode(rel_op), cmp);    
}

Terminal Parser::relation_op() {
    return check_and_next_return<Terminal>("Expected terminal in relation!");
}

/* Base Parsing */
instruct_t Parser::expression(const bb_t& curr_block) {
    instruct_t result = term(curr_block);
    while(is_terminal(lexer.token, Terminal::PLUS, Terminal::MINUS)) { 
        if(is_terminal(lexer.token, Terminal::PLUS)){
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
    while(is_terminal(lexer.token, Terminal::MUL, Terminal::DIV)) {
        if(is_terminal(lexer.token, Terminal::MUL)) {
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
    if(lexer.token.type == TokenType::USER_IDENTIFIER){
        result = ir.get_ident_value(curr_block, std::get<ident_t>(lexer.token.payload));
        lexer.next();
        return result;
    } 
    else if(lexer.token.type == TokenType::CONSTANT) {
        result = ir.add_instruction(const_block, Opcode::CONST, std::get<int>(lexer.token.payload));        
        lexer.next();
        return result;
    }
    else if (is_terminal(lexer.token, Terminal::LPAREN)) {
        lexer.next();
        result = expression(curr_block);
        if(is_terminal(lexer.token, Terminal::RPAREN))
            lexer.next();
        return result;
    }
    throw ParserException("Expected user identifier, constant or '(' in factor.");
}
