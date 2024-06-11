#include "parser.hpp"
#include "token.hpp"
#include <format>
#include <ranges>

/* API */
void Parser::parse() {
    computation();
}

Parser::Parser(std::istream& in) : lexer(in) {}

void Parser::print() {
    std::cout << ir.to_dotlang();
}

IntermediateRepresentation Parser::release_ir() {
    return ir;
}

/* Parsing main function of Tiny program */
void Parser::computation() {
    /* "main" [ variable_declaration ] { function_declaration } "{" statSequence "}" "." */
    match(Keyword::MAIN);
    variable_declaration();
    bb_t curr_block = function_declaration();
    match(Terminal::LBRACE);
    statement_sequence(curr_block, Terminal::RBRACE);
    match(Terminal::RBRACE);
    match(Terminal::PERIOD);
    lexer.check_all_defined();
}

/* Declarations */
void Parser::variable_declaration() {
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
}

bb_t Parser::function_declaration() {
    /* [ "void" ] "function" ident formal_parameters ";" function_body ";" */ 
    // these are declared vars of the main function. 
    std::vector<std::string> saved_identifier_strings = lexer.wipe();     

    // contains identifier strings for previously defined functions. (e.g., "function foo()" would have "foo" in this vector)
    std::vector<std::string> func_strings{}; 

    // contains instruction numbers of the first instruction of previously defined functions.
    std::vector<instruct_t> func_first_instructs{};

    while(token_is(lexer.token, Keyword::VOID, Keyword::FUNCTION)) {
        // "void"
        bool void_func = token_is(lexer.token, Keyword::VOID);
        if(void_func) lexer.next();

        // "function"
        lexer.wipe();
        lexer.insert_ident(func_strings);
        match(Keyword::FUNCTION);
        match<ident_t>();
        ++lexer.func_count;
        func_strings.emplace_back(lexer.last_ident_string);

        // formal_parameters ";" function_body ";"
        std::vector<ident_t> formal_params = formal_parameters();
        match(Terminal::SEMICOLON);
        func_first_instructs.emplace_back(function_body(formal_params, func_first_instructs));
        match(Terminal::SEMICOLON);
        lexer.check_all_defined();
    }

    // Establish main function
    lexer.wipe();
    lexer.insert_ident(std::move(func_strings));
    lexer.insert_ident(std::move(saved_identifier_strings));
    bb_t main_block = ir.new_function(const_block, lexer.ident_index);

    // Make all the inserted identifiers of previously defined functions point to their first instruction
    ident_t index = 0;
    for(const auto& first_instruct : func_first_instructs) {
        ir.change_ident_value(main_block, index, first_instruct);
        ++index;
    }
    return main_block;
}

std::vector<ident_t> Parser::formal_parameters() {
    // "(" [ ident { "," ident } ] ")"
    std::vector<ident_t> formal_params{};
    match(Terminal::LPAREN);
    if(!token_is(lexer.token, Terminal::RPAREN)) {
        ident_t param = match_return<ident_t>();
        formal_params.emplace_back(param);
        lexer.set_defined(param);
    }
    while(!token_is(lexer.token, Terminal::RPAREN)) {
        match(Terminal::COMMA);
        ident_t param = match_return<ident_t>();
        formal_params.emplace_back(param);
        lexer.set_defined(param);
    }
    match(Terminal::RPAREN);
    return formal_params;
}

instruct_t Parser::function_body(const std::vector<ident_t>& formal_params, const std::vector<instruct_t>& func_first_instructs) {
    /* [ variable_declaration ] "{" statement_sequence "}" */
    variable_declaration();
    bb_t func_block = ir.new_function(const_block, lexer.ident_index);
    const bb_t og_func_block = func_block;

    // Make all previously defined functions point to their first instruction
    ident_t index = 0;
    for(const auto& first_instruct : func_first_instructs) {
        ir.change_ident_value(func_block, index, first_instruct);
        ++index;
    }
    ir.change_ident_value(func_block, index, ir.first_instruction(func_block));

    // Add GETPAR instructions (reversed since we're popping from a stack)
    for(const auto& param : formal_params | std::views::reverse) {
        ir.change_ident_value(func_block, param, ir.add_instruction(func_block, Opcode::GETPAR));
    }

    match(Terminal::LBRACE);
    statement_sequence(func_block, Terminal::RBRACE);
    match(Terminal::RBRACE);
    if(!ir.has_branch_instruction(func_block)) {
        ir.set_branch_cond(func_block, Opcode::RET, -1);
    }
    return ir.first_instruction(og_func_block);
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

        // Enforcing semicolons for all but the last statement.
        // if(!token_is(lexer.token, Terminal::SEMICOLON, Terminal::RBRACE) && 
        //    !token_is(lexer.token,  Keyword::ELSE, Keyword::FI, Keyword::OD)) {
        //     throw ParserException("Missing semicolon!");
        // } else if(token_is(lexer.token, Terminal::SEMICOLON)) {
        //     lexer.next();
        // }

        // Could use this instead. Wont enforce semicolons for all but the last statement.
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
            function_statement(curr_block);
            return false;
        case Keyword::IF:
            return if_statement(curr_block);
        case Keyword::WHILE:
            return while_statement(curr_block);
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
    ir.change_ident_value(curr_block, ident, expression(curr_block).first);
    lexer.set_defined(ident);
}

std::pair<instruct_t, ident_t> Parser::predefined_function_statement(const bb_t& curr_block) {
    std::pair<instruct_t, ident_t> result;
    switch(match_return<Keyword>()) {
        case Keyword::READ:
            if(token_is(lexer.token, Terminal::LPAREN)) {
                lexer.next();
                match(Terminal::RPAREN);
            }
            return { ir.add_instruction(curr_block, Opcode::READ), -1 };
        case Keyword::WRITE:
            match(Terminal::LPAREN);
            result = { ir.add_instruction(curr_block, Opcode::WRITE, expression(curr_block)), -1 };
            match(Terminal::RPAREN);
            return result;
        case Keyword::WRITENL:
            if(token_is(lexer.token, Terminal::LPAREN)) {
                lexer.next();
                match(Terminal::RPAREN);
            }
            return { ir.add_instruction(curr_block, Opcode::WRITENL), -1 };
        default:
            throw ParserException(std::format("Invalid reserved keyword in function call! Received {}", to_string(lexer.token)));
    }
}

std::pair<instruct_t, ident_t> Parser::function_statement(const bb_t& curr_block) {
    /* ident [ "(" [ expression { "," expression } ] ")" ] */
    // Predefined functions
    if(token_is<Keyword>(lexer.token)) {
        return predefined_function_statement(curr_block);
    }
    
    // User defined functions
    ident_t ident = match_return<ident_t>();
    if(token_is(lexer.token, Terminal::LPAREN)) {
        lexer.next();
        if(!token_is(lexer.token, Terminal::RPAREN)) {
            ir.add_instruction(curr_block, Opcode::SETPAR, expression(curr_block));
        }
        while(!token_is(lexer.token, Terminal::RPAREN)) {
            match(Terminal::COMMA);
            ir.add_instruction(curr_block, Opcode::SETPAR, expression(curr_block));
        }
        match(Terminal::RPAREN);
    }
    return { ir.add_instruction(curr_block, Opcode::JSR, ir.get_ident_value(curr_block, ident)), -1 };
}

bool Parser::if_statement(bb_t& curr_block) {
    /* relation "then" statement_sequence [ "else" statement_sequence ] "fi" */
    Relation result = relation(curr_block, false);

    // "then"
    match(Keyword::THEN);
    bb_t then_block = result != Relation::NEITHER ? curr_block : ir.new_block(curr_block, IF_FALLTHROUGH);
    if(result == Relation::TRUE) {
        bool prev_ignore = ir.ignore;
        ir.ignore = true;
        bb_t dummy = -1;
        statement_sequence(dummy, Keyword::ELSE, Keyword::FI);
        ir.ignore = prev_ignore;
    } else {
        statement_sequence(then_block, Keyword::ELSE, Keyword::FI);
    }

    // "else"
    bb_t else_block = result != Relation::NEITHER ? curr_block : ir.new_block(curr_block, IF_BRANCH);
    const bb_t og_else_block = else_block;
    if(token_is(lexer.token, Keyword::ELSE)) {
        lexer.next();   
        if(result == Relation::FALSE) {
            bool prev_ignore = ir.ignore;
            ir.ignore = true;
            bb_t dummy = -1;
            statement_sequence(dummy, Keyword::FI);
            ir.ignore = prev_ignore;
        } else {
            statement_sequence(else_block, Keyword::FI);
        }
    }
    match(Keyword::FI);

    if(result == Relation::NEITHER) {
        // Set branch location for dominator block
        ir.set_branch_location(curr_block, ir.first_instruction(og_else_block));
    }

    // If both blocks return, no need to continue.
    if(ir.will_return(then_block) && ir.will_return(else_block)) {
         ir.set_return(curr_block);
         return true;
    }

    // If one of the blocks returns, no need to join.
    if(ir.will_return(then_block) || result == Relation::TRUE) {
         curr_block = else_block;
         return false;
    } else if (ir.will_return(else_block) || result == Relation::FALSE) {
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

bool Parser::while_statement(bb_t& curr_block) {
    /* "while" relation "do" statement_sequence "od" */
    curr_block = ir.new_block(curr_block, Blocktype::LOOP_HEADER); // Loop header

    // Relation
    Relation result = relation(curr_block, true);

    bb_t while_block = result == Relation::TRUE ? -1 : ir.new_block(curr_block, WHILE_FALLTHROUGH);
    // "do" statement_sequence "od"
    match(Keyword::DO);
    bool prev_while_loop = ir.while_loop;
    ir.while_loop = true;
    if(result == Relation::TRUE) {
        bool prev_ignore = ir.ignore;
        ir.ignore = true;
        bb_t dummy = -1;
        statement_sequence(dummy, Keyword::OD);
        ir.ignore = prev_ignore;
    } else {
        statement_sequence(while_block, Keyword::OD);
    }
    match(Keyword::OD);
    ir.while_loop = prev_while_loop;

    if(result == Relation::FALSE) {
        // ir.generate_phi(curr_block, while_block);
        ir.update_phi(curr_block, while_block);
        ir.set_branch_cond(while_block, Opcode::BRA, ir.first_instruction(curr_block));
        if(!ir.while_loop) ir.commit_while(curr_block, curr_block, while_block, lexer.identifier_table); 
        curr_block = while_block;
        return true;
    } else if(result == Relation::TRUE) {
        return false;
    }

    // Create BRANCH block
    branch(curr_block, while_block);
    return false;
}

void Parser::branch(bb_t& curr_block, const bb_t& while_block) {
    bb_t og_curr_block = curr_block;
    // ir.generate_phi(og_curr_block, while_block);
    ir.update_phi(og_curr_block, while_block);
    ir.set_branch_cond(while_block, Opcode::BRA, ir.first_instruction(og_curr_block));
    if(!ir.while_loop) ir.commit_while(curr_block, curr_block, while_block, lexer.identifier_table);
    curr_block = ir.new_block(curr_block, WHILE_BRANCH);
    ir.set_branch_location(og_curr_block, ir.first_instruction(curr_block));
}

void Parser::return_statement(bb_t& curr_block) {
    // [ expression ]
    // Check for a potential expression after "return"
    if(token_is(lexer.token, Keyword::CALL) ||
       token_is(lexer.token, Terminal::LPAREN, Terminal::RPAREN) ||
       token_is<int>(lexer.token) ||
       token_is<ident_t>(lexer.token)) {
        std::pair<instruct_t, ident_t> expr = expression(curr_block);
        ir.set_branch_cond(curr_block, Opcode::RET, expr);
        return;
    }
    ir.set_branch_cond(curr_block, Opcode::RET, -1);
}

/* Relations */
Relation Parser::relation(const bb_t& curr_block, const bool& while_statement) {
    /* expression1 rel_op expression2 */
    std::pair<instruct_t, ident_t> larg = expression(curr_block);
    Terminal rel_op = match_return<Terminal>();
    std::pair<instruct_t, ident_t> rarg = expression(curr_block);

    // Constant folding (literals)
    if(ir.is_const_instruction(larg.first) && ir.is_const_instruction(rarg.first) && larg.second == -1 && rarg.second == -1) {
        return terminal_cmp(rel_op, ir.get_const_value(larg.first), ir.get_const_value(rarg.first)) ? Relation::TRUE : Relation::FALSE;
    }

    // Non-nested if statement constant folding (non-literals, skip on true and false)
    if(!while_statement && !ir.while_loop && ir.is_const_instruction(larg.first) && ir.is_const_instruction(rarg.first)) {
        return terminal_cmp(rel_op, ir.get_const_value(larg.first), ir.get_const_value(rarg.first)) ? Relation::TRUE : Relation::FALSE;
    }

    // Non-nested while statement constant folding (non-literals, skip only if false)
    if(while_statement && !ir.while_loop && ir.is_const_instruction(larg.first) && ir.is_const_instruction(rarg.first) &&
       terminal_cmp(rel_op, ir.get_const_value(larg.first), ir.get_const_value(rarg.first)) == true) {
        return Relation::TRUE;
    }
    
    // Set up CMP instruction and branch instruction.
    instruct_t cmp = ir.add_instruction(curr_block, Opcode::CMP, larg, rarg);
    ir.set_branch_cond(curr_block, terminal_to_opcode(rel_op), cmp);
    return Relation::NEITHER;
}

/* Base Parsing */
std::pair<instruct_t, ident_t> Parser::expression(const bb_t& curr_block) {
    std::pair<instruct_t, ident_t> larg = term(curr_block);
    while(token_is(lexer.token, Terminal::PLUS, Terminal::MINUS)) {
        operate(curr_block, larg, &Parser::term); 
    }
    return larg;
}

std::pair<instruct_t, ident_t> Parser::term(const bb_t& curr_block) {
    std::pair<instruct_t, ident_t> larg = factor(curr_block);
    while(token_is(lexer.token, Terminal::MUL, Terminal::DIV)) {
        operate(curr_block, larg, &Parser::factor);
    }
    return larg;
}

void Parser::operate(const bb_t& curr_block, std::pair<instruct_t, ident_t>& larg, std::pair<instruct_t, ident_t>(Parser::*func)(const bb_t&)) {
    // Grab terminal
    auto operation = operations_map.find(match_return<Terminal>());
    if(operation == operations_map.end()) {
        throw ParserException("Invalid terminal operation call!");
    }

    // Get rarg
    std::pair<instruct_t, ident_t> rarg = (this->*func)(curr_block);

    // Operation (either const folding or non-const)
    if (ir.is_const_instruction(larg.first) && ir.is_const_instruction(rarg.first) &&
         (!ir.while_loop || (ir.while_loop && larg.second == -1 && rarg.second == -1))) {
        larg = { ir.add_instruction(const_block, Opcode::CONST, operation->second.second(ir.get_const_value(larg.first), ir.get_const_value(rarg.first))), -1 };
    } else {
        larg = { ir.add_instruction(curr_block, operation->second.first, larg, rarg), -1 };
    }
}

std::pair<instruct_t, ident_t> Parser::factor(const bb_t& curr_block) {
    ident_t ident;
    if(token_is<ident_t>(lexer.token)) {
        ident = match_return<ident_t>();
        return { ir.get_ident_value(curr_block, ident), ident };
    } else if (token_is<int>(lexer.token)) {
        return { ir.add_instruction(const_block, Opcode::CONST, match_return<int>()), -1 }; 
    } else if (token_is(lexer.token, Terminal::LPAREN)) {
        lexer.next();
        std::pair<instruct_t, ident_t> result = expression(curr_block);
        match(Terminal::RPAREN);
        return result;
    } else if (token_is(lexer.token, Keyword::CALL)) {
        lexer.next();
        return function_statement(curr_block);
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

bool Parser::terminal_cmp(const Terminal& terminal, const int& larg, const int& rarg) {
    switch(terminal) {
        case Terminal::EQ:
            return larg != rarg;
        case Terminal::NEQ:
            return larg == rarg;
        case Terminal::LT:
            return larg >= rarg;
        case Terminal::LE:
            return larg > rarg;
        case Terminal::GT:
            return larg <= rarg;
        case Terminal::GE:
            return larg < rarg;
        default:
            throw ParserException("Invalid terminal to opcode conversion!");
    }
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
