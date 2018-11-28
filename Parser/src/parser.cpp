/**
 * |-------------|
 * | Nuua Parser |
 * |-------------|
 *
 * Copyright 2018 Erik Campobadal <soc@erik.cat>
 * https://nuua.io
 */
#include "../include/parser.hpp"
#include "../../Lexer/include/lexer.hpp"
#include "../../Logger/include/logger.hpp"

#define CURRENT() (*(this->current))
#define PREVIOUS() (*(this->current - 1))
#define CHECK(token) (token == CURRENT().type)
#define NEXT() (*(this->current++))
#define IS_AT_END() (this->current->type == TOKEN_EOF)
#define LOOKAHEAD(n) (*(this->current + n))

const std::unordered_map<char, char> Parser::escapedChars = {
    { '\\', '\\' },
    { '\'', '\'' },
    { '"', '"' },
    { 'n', '\n' },
    { 't', '\t'},
    { 'r', '\r'},
    { 'b', '\b'},
    { 'f', '\f'},
    { 'v', '\v'},
    { '0', '\0'}
};

Token Parser::consume(TokenType type, const char* message)
{
    if (this->current->type == type) return NEXT();

    logger->error(std::string(message), this->current->line);
    exit(EXIT_FAILURE);
}

bool Parser::match(TokenType token)
{
    if (CHECK(token)) {
        if (token != TOKEN_EOF) NEXT();
        return true;
    }

    return false;
}

bool Parser::matchAny(std::vector<TokenType> tokens)
{
    for (auto token : tokens) {
        if (CHECK(token)) {
            if (token != TOKEN_EOF) NEXT();
            return true;
        }
    }

    return false;
}

std::vector<Statement *> Parser::getBlockBody()
{
    std::vector<Statement *> body;
    while (!CHECK(TOKEN_RIGHT_BRACE) && !IS_AT_END()) {
        body.push_back(this->statement());
    }
    return body;
}

std::string Parser::toString(Token token)
{
    std::string s;
    for (uint32_t i = 0; i < token.length; i++) {
        auto c = token.start + i;
        s += *c;
        if (*c == '\\') {
            auto nc = *(c + 1);
            if (Parser::escapedChars.find(nc) != Parser::escapedChars.end()) {
                s.pop_back();
                s += Parser::escapedChars.at(nc);
                i++; // Increment i to avoid char repetition
            }
        }
    }

    return s;
}

bool Parser::isFunction()
{
    uint32_t current = 0;
    for (uint32_t skip = 0; skip == 0 && LOOKAHEAD(current).type != TOKEN_RIGHT_PAREN; current++) {
        if (LOOKAHEAD(current).type == TOKEN_EOF) {
            logger->error("Unterminated grouping or function. Did you close the parenthesis?", this->current->line);
            exit(EXIT_FAILURE);
        }
        else if (LOOKAHEAD(current).type == TOKEN_LEFT_PAREN) skip++;
        else if (LOOKAHEAD(current).type == TOKEN_RIGHT_PAREN) skip--;
    }
    if (LOOKAHEAD(current + 1).type == TOKEN_EOF) return false;

    return LOOKAHEAD(current + 1).type == TOKEN_LEFT_BRACE
        || LOOKAHEAD(current + 1).type == TOKEN_RIGHT_ARROW
        || LOOKAHEAD(current + 1).type == TOKEN_BIG_RIGHT_ARROW;
}

Expression *Parser::function()
{
    std::vector<Expression *> arguments;
    std::vector<Statement *> body;

    // Get the function arguments
    if (!CHECK(TOKEN_RIGHT_PAREN)) {
        do {
            Expression *argument = this->expression();

            // Check if it's a correct argument type
            // Checked already when compiling :)
            /*
            switch (argument->type) {
                case RULE_VARIABLE: { break; }
                default: { logger->error("Invalid argument type when defining the function", this->current->line); exit(EXIT_FAILURE); }
            }
            */

            arguments.push_back(argument);
        } while (this->match(TOKEN_COMMA));
    }

    this->consume(TOKEN_RIGHT_PAREN, "Expected ')' after function arguments");

    // Get the function body
    if (this->match(TOKEN_LEFT_BRACE)) {
        this->consume(TOKEN_NEW_LINE, "Expected a new line after the '{'");
        body = this->getBlockBody();
        this->consume(TOKEN_RIGHT_BRACE, "Unterminated block. Expected '}'");
        // This is checked already since it's an expression statement
        // this->consume(TOKEN_NEW_LINE, "Expected a new line after the '}'");
    } else {
        logger->error("Unknown function body found. You need to use '{' or '=>' or '->' after the arguments to define the function's body.", this->current->line);
        exit(EXIT_FAILURE);
    }

    return new Function(arguments, body);
}

Expression *Parser::list()
{
    std::vector<Expression *> values;
    if (this->match(TOKEN_RIGHT_SQUARE)) return new List(values);
    for (;;) {
        if (IS_AT_END()) {
            logger->error("Unfinished list, Expecting ']' after the last list element.", this->current->line);
            exit(EXIT_FAILURE);
        }
        values.push_back(expression());
        if (this->match(TOKEN_RIGHT_SQUARE)) break;
        this->consume(TOKEN_COMMA, "Expected ',' after list element");
    }

    return new List(values);
}

Expression *Parser::dictionary()
{
    std::unordered_map<std::string, Expression *> values;
    std::vector<std::string> keys;
    if (this->match(TOKEN_RIGHT_BRACE)) return new Dictionary(values, keys);
    for (;;) {
        if (IS_AT_END()) {
            logger->error("Unfinished dictionary, Expecting '}' after the last dictionary element.", this->current->line);
            exit(EXIT_FAILURE);
        }
        auto key = this->expression();
        if (key->rule != RULE_VARIABLE) {
            logger->error("Expected an identifier as a key", this->current->line);
            exit(EXIT_FAILURE);
        }
        this->consume(TOKEN_COLON, "Expected ':' after dictionary key");
        auto name = static_cast<Variable *>(key)->name;
        values[name] = this->expression();
        keys.push_back(name);
        if (this->match(TOKEN_RIGHT_BRACE)) break;
        this->consume(TOKEN_COMMA, "Expected ',' after dictionary element");
    }

    /*
    printf("-\n");
    for (auto a : values) printf("%s\n", a.first.c_str());
    printf("-\n");
    */

    return new Dictionary(values, keys);
}

Expression *Parser::primary()
{
    // debug_token(CURRENT());
    if (this->match(TOKEN_FALSE)) return new Boolean(false);
    if (this->match(TOKEN_TRUE)) return new Boolean(true);
    if (this->match(TOKEN_NONE)) return new None();
    if (this->match(TOKEN_NUMBER)) return new Number(std::stof(this->toString(PREVIOUS())));
    if (this->match(TOKEN_STRING)) return new String(this->toString(PREVIOUS()));
    if (this->match(TOKEN_IDENTIFIER)) return new Variable(this->toString(PREVIOUS()));
    if (this->match(TOKEN_LEFT_SQUARE)) return this->list();
    if (this->match(TOKEN_LEFT_BRACE)) return this->dictionary();
    if (this->match(TOKEN_LEFT_PAREN)) {
        if (this->isFunction()) return this->function();
        auto value = this->expression();
        this->consume(TOKEN_RIGHT_PAREN, "Expected ')' after a group expression");

        return new Group(value);
    }
    // printf("FAILED AT: ");
    // debug_token(CURRENT());
    logger->error("Expected an expression", this->current->line);
    exit(EXIT_FAILURE);
}

Expression *Parser::finishCall(Expression *callee)
{
    if (callee->rule != RULE_VARIABLE) {
        logger->error("Expected an identifier as the function name.", this->current->line);
        exit(EXIT_FAILURE);
    }
    std::vector<Expression *> arguments;
    if (!CHECK(TOKEN_RIGHT_PAREN)) {
        arguments.push_back(this->expression());
        while (this->match(TOKEN_COMMA) && !IS_AT_END()) arguments.push_back(this->expression());
    }

    this->consume(TOKEN_RIGHT_PAREN, "Expected ')' after call arguments");

    return new Call(static_cast<Variable *>(callee)->name, arguments);
}

Expression *Parser::finishAccess(Expression *item)
{
    if (item->rule != RULE_VARIABLE) {
        logger->error("Expected an identifier as the access variable.", this->current->line);
        exit(EXIT_FAILURE);
    }
    auto index = this->expression();
    this->consume(TOKEN_RIGHT_SQUARE, "Expected ']' after index access");

    return new Access(static_cast<Variable *>(item)->name, index);
}

Expression *Parser::call()
{
    auto result = this->primary();
    for (;;) {
        if (this->match(TOKEN_LEFT_PAREN)) {
            result = this->finishCall(result);
        } else if (this->match(TOKEN_LEFT_SQUARE)) {
            result = this->finishAccess(result);
        } else {
            break;
        }
    }

    return result;
}

Expression *Parser::unary()
{
    if (this->matchAny(std::vector<TokenType>({ TOKEN_BANG, TOKEN_MINUS }))) {
        auto op = PREVIOUS();
        return new Unary(op, this->unary());
    }

    return this->call();
}

Expression *Parser::mulDivMod()
{
    auto result = this->unary();
    while (this->matchAny(std::vector<TokenType>({ TOKEN_SLASH, TOKEN_STAR, TOKEN_PERCENT }))) {
        auto op = PREVIOUS();
        result = new Binary(result, op, this->unary());
    }

    return result;
}

Expression *Parser::addition()
{
    auto result = this->mulDivMod();
    while (this->matchAny(std::vector<TokenType>({ TOKEN_MINUS, TOKEN_PLUS }))) {
        auto op = PREVIOUS();
        result = new Binary(result, op, this->mulDivMod());
    }

    return result;
}

Expression *Parser::comparison()
{
    auto result = this->addition();
    while (this->matchAny(std::vector<TokenType>({ TOKEN_HIGHER, TOKEN_HIGHER_EQUAL, TOKEN_LOWER, TOKEN_LOWER_EQUAL }))) {
        auto op = PREVIOUS();
        result = new Binary(result, op, this->addition());
    }

    return result;
}

Expression *Parser::equality()
{
    auto result = this->comparison();
    while (this->matchAny(std::vector<TokenType>({ TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL }))) {
        auto op = PREVIOUS();
        result = new Binary(result, op, this->comparison());
    }

    return result;
}

Expression *Parser::andOperator()
{
    auto result = this->equality();
    while (this->match(TOKEN_AND)) {
        auto op = PREVIOUS();
        result = new Logical(result, op, this->equality());
    }

    return result;
}

Expression *Parser::orOperator()
{
    auto result = this->andOperator();
    while (this->match(TOKEN_AND)) {
        auto op = PREVIOUS();
        result = new Logical(result, op, this->andOperator());
    }

    return result;
}

Expression *Parser::assignment()
{
    auto result = this->orOperator();
    if (this->match(TOKEN_EQUAL)) {
        switch (result->rule) {
            case RULE_VARIABLE: { return new Assign(static_cast<Variable *>(result)->name, this->expression()); }
            case RULE_ACCESS: {
                printf("Found assign access.\n");
                auto res = static_cast<Access *>(result);
                return new AssignAccess(res->name, res->index, this->expression());
            }
            default: { logger->error("Invalid assignment target", this->current->line); exit(EXIT_FAILURE); };
        }
    }
    return result;
}

Expression *Parser::expression()
{
    return this->assignment();
}

Statement *Parser::expressionStatement()
{
    auto expr = this->expression();
    if (!this->matchAny(std::vector<TokenType>({ TOKEN_NEW_LINE, TOKEN_EOF }))) {
        logger->error("Expected a new line or EOF after expression statement", this->current->line);
        exit(EXIT_FAILURE);
    }
    return new ExpressionStatement(expr);
}

Statement *Parser::ifStatement()
{
    this->consume(TOKEN_LEFT_PAREN, "Expected a '(' after 'if'");
    auto condition = this->expression();
    this->consume(TOKEN_RIGHT_PAREN, "Expected a ')' after 'if' condition");
    this->consume(TOKEN_LEFT_BRACE, "Expected a '{' after the ')'");
    this->consume(TOKEN_NEW_LINE, "Expected a new line after the '{'");
    auto thenBranch = this->getBlockBody();
    this->consume(TOKEN_RIGHT_BRACE, "Unterminated block. Expected '}'");
    if (!this->matchAny(std::vector<TokenType>({ TOKEN_NEW_LINE, TOKEN_EOF }))) {
        logger->error("Expected a new line after the '}'", this->current->line);
        exit(EXIT_FAILURE);
    }

    return new If(condition, thenBranch, std::vector<Statement *>({}));
}

Statement *Parser::whileStatement()
{
    this->consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'");
    auto condition = this->expression();
    this->consume(TOKEN_RIGHT_PAREN, "Expected ')' after 'while' condition");
    this->consume(TOKEN_LEFT_BRACE, "Expected a '{' after the ')'");
    this->consume(TOKEN_NEW_LINE, "Expected a new line after the '{'");
    auto body = this->getBlockBody();
    this->consume(TOKEN_RIGHT_BRACE, "Unterminated block. Expected '}'");
    if (!this->matchAny(std::vector<TokenType>({ TOKEN_NEW_LINE, TOKEN_EOF }))) {
        logger->error("Expected a new line after the '}'", this->current->line);
        exit(EXIT_FAILURE);
    }
    return new While(condition, body);
}

Statement *Parser::statement()
{
    if (this->match(TOKEN_IF)) return this->ifStatement();
    else if (this->match(TOKEN_WHILE)) return this->whileStatement();

    return this->expressionStatement();
}

std::vector<Statement *> Parser::parse(const char *source)
{
    auto lexer = new Lexer;

    std::vector<Token> tokens = lexer->scan(source);

    delete lexer;

    logger->info("Started parsing...");

    this->current = &tokens.front();

    std::vector<Statement *> code;

    while (!IS_AT_END()) {
        // printf("Looking for statement... Starting at ");
        // CURRENT().debug_token();
        code.push_back(this->statement());
    }

    Parser::debug_rules(code);

    logger->success("Parsing completed");

    return code;
}

#undef CURRENT
#undef PREVIOUS
#undef CHECK
#undef NEXT
#undef IS_AT_END
#undef LOOKAHEAD
