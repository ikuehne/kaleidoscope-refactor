#include "Parser.hh"

#include <iostream>
#include <map>
#include <vector>

namespace Kaleidoscope {

/*****************************************************************************
 * Utilities.
 */

/** Associate operators with their precedence. */
static const std::map<char, int> BINOP_PRECEDENCE {
    {'<', 10},
    {'+', 20},
    {'-', 20},
    {'*', 40},
    {'/', 40},
};

/* Look up the precedence of the current token. */
int Parser::get_token_precedence(void) const {
    if (!isascii(cur_token))
        return -1;

    /* Can't just use operator[] because `BINOP_PRECEDENCE` is `const`. */
    if (BINOP_PRECEDENCE.count(cur_token) == 0) return -1;

    // Make sure it's a declared binop.
    int result = BINOP_PRECEDENCE.find(cur_token)->second;

    return result;
}

/* Log an error to stderr, and return an error code. */
static AST::Expression log_error(const char *str) {
    std::cerr << "Kaleidoscope::Parser::log_error: "
              << str << std::endl;
    return AST::Error{};
}

/* Log an error to stderr, and return an error code. */
static std::unique_ptr<AST::FunctionPrototype> log_error_p(const char *s) {
    log_error(s);
    return nullptr;
}

Parser::Parser(std::string input): lexer(input) {
    shift_token();
}

int Parser::shift_token(void) {
    /* `cur_token` gives us 1 token of lookahead. */
    cur_token = lexer.get_token();
    return cur_token;
}

AST::NumberLiteral Parser::parse_number(void) {
    AST::NumberLiteral result(lexer.get_number());
    /* Advance the lexer. */
    shift_token();
    return result;
}

AST::Expression Parser::parse_parens(void) {
    /* Shift the opening paren. */
  	shift_token();
    /* Get the body of the expression. */
    auto contents = parse_expression();

    if (AST::is_err(contents)) return contents;

    /* If it didn't end in a close paren, error. */
    if (cur_token != ')') return log_error("expected ')'");

    /* Shift the closing paren. */
    shift_token();
    return contents;
}

AST::Expression Parser::parse_identifier(void) {
    /* Get the identifier. */
    std::string id = lexer.get_identifier();

    /* Shift the identifier. */
    shift_token();

    /* Unless this is a function call, */
    if (cur_token != '(')
        /* it's a variable. */
        return id;

    /* If it is a function call, shift the opening paren.*/
    shift_token();
    /* Collect an argument vector. */
    std::vector<AST::Expression> args;
    /* (unless there are no arguments). */
    if (cur_token != ')') {
        while (1) {
            AST::Expression arg(parse_expression());
            if (!AST::is_err(arg)) {
                args.push_back(std::move(arg));
            } else {
                return arg;
            }

            if (cur_token == ')') break;

            if (cur_token != ',') {
                return log_error("Expected ')' or ',' in argument list");
            }

            shift_token();
        }
    }

    // Eat the ')'.
    shift_token();

    return std::make_unique<AST::FunctionCall>(id,
                                               std::move(args));
}

AST::Expression Parser::parse_primary(void) {
    AST::Expression result(AST::Error{});
    switch (cur_token) {
        case tok_identifier:
            return parse_identifier();
        case tok_number:
            return parse_number();
        case '(':
            return parse_parens();
        case tok_if:
            return parse_if_then_else();
        case tok_for:
            return parse_for_loop();
        default:
            result = log_error("Unknown token when expecting expression.");
            assert(AST::is_err(result));
            return result;
    }
}

AST::Expression Parser::parse_expression(void) {
    auto lhs = parse_primary();
    if (AST::is_err(lhs)) return lhs;
    return parse_binop_rhs(0, std::move(lhs));
}

AST::Expression Parser::parse_binop_rhs(int prec, AST::Expression lhs) {

    while (true) {
        int op_prec = get_token_precedence();

        /* If the precedence on the right is lower than on the left, return
         * the expression on the left. */
        if (op_prec < prec) return lhs;

        /* Save and shift the current token. */
        int op = cur_token;
        shift_token();

        /* Parse the right-hand side. */
        auto rhs = parse_primary();
        if (AST::is_err(rhs)) return rhs;

        /* Check the precedence of the next operator.  (Note that if the next
         * token is *not* an operator, `get_token_precedence` will return -1).
        */
        int next_prec = get_token_precedence();
        if (op_prec < next_prec) {
            rhs = parse_binop_rhs(op_prec + 1, std::move(rhs));

            if (AST::is_err(rhs)) return rhs;
        }

        lhs = std::make_unique<AST::BinaryOp>(op, std::move(lhs),
                                                  std::move(rhs));
    }
}

AST::Expression Parser::parse_if_then_else(void){
    /* Shift "if". */
    shift_token();

    auto cond = parse_expression();
    if (AST::is_err(cond)) return cond;
    if (cur_token != tok_then) return log_error("expected \"then\".");

    /* Shift "then". */
    shift_token();
    auto then = parse_expression();
    if (AST::is_err(then)) return then;
    if (cur_token != tok_else) return log_error("expected \"else\".");

    /* Shift "else". */
    shift_token();
    auto _else = parse_expression();
    if (AST::is_err(_else)) return _else;

    return std::make_unique<AST::IfThenElse>(std::move(cond),
                                             std::move(then),
                                             std::move(_else));
}

AST::Expression Parser::parse_for_loop(void) {
    /* Shift "for". */
    shift_token();

    if (cur_token != tok_identifier) {
        return log_error("expected identifier as loop index.");
    }

    std::string idx = lexer.get_identifier();

    shift_token();

    if (cur_token != '=') return log_error("expected '=' in loop.");

    shift_token();

    auto start = parse_expression();

    if (cur_token != ',') {
        return log_error("expected ',' between loop elements.");
    }

    shift_token();

    auto term = parse_expression();

    AST::Expression incr(AST::NumberLiteral(1.0));
    if (cur_token == ',') {
        shift_token();
        incr = parse_expression();
    }

    if (cur_token != tok_in) {
        return log_error("Expected 'in' after for loop.");
    }

    shift_token();

    auto body = parse_expression();

    return std::make_unique<AST::ForLoop>(idx, std::move(start),
                                               std::move(term),
                                               std::move(incr),
                                               std::move(body));
}

std::unique_ptr<AST::FunctionPrototype> Parser::parse_prototype(void) {
    if (cur_token != tok_identifier)
        return log_error_p("Expected function name in prototype");

    std::string fname = lexer.get_identifier();
    shift_token();

    if (cur_token != '(')
        return log_error_p("Expected '(' in prototype");

    /* Read the list of argument names. */
    std::vector<std::string> args;
    while (shift_token() == tok_identifier)
        args.push_back(lexer.get_identifier());
    if (cur_token != ')')
        return log_error_p("Expected ')' in prototype");

    /* Shift the closing parenthesis. */
    shift_token();

    return std::make_unique<AST::FunctionPrototype>(fname, std::move(args));
}

AST::Declaration Parser::parse_definition(void) {
    /* Shift "def". */
    shift_token();
    /* Get the prototype. */
    auto proto = parse_prototype();
    if (!proto) return AST::Error{};

    /* Get the body. */
    auto body = parse_expression();
    if (AST::is_err(body)) return AST::Error{};


    return std::make_unique<AST::FunctionDefinition>(std::move(proto),
                                                     std::move(body));
}

AST::Declaration Parser::parse_extern(void) {
    /* Shift "extern". */
    shift_token();
    /* Other than that, an extern is a normal prototype.*/
    auto result = parse_prototype();

    if (!result) return AST::Error{};
    return std::move(result);
}

AST::Declaration Parser::parse_top_level(void) {
    /* Parse the expression. */
    auto expr = parse_expression();
    if (AST::is_err(expr)) return AST::Error{};

    /* Turn it into the body of an anonymous prototype. */
    auto proto =
        std::make_unique<AST::FunctionPrototype>("",
                                                 std::vector<std::string>());
    return std::make_unique<AST::FunctionDefinition>(
            AST::FunctionDefinition(std::move(proto), std::move(expr)));
}

AST::Declaration Parser::parse(void) {
    AST::Declaration result;
    switch (cur_token) {
    case tok_eof:
        std::cerr << "Hit end of file." << std::endl;
        return AST::Error{};
    case ';': // ignore top-level semicolons.
        shift_token();
        break;
    case tok_def:
        result = parse_definition();
        break;
    case tok_extern:
        result = parse_extern();
        break;
    default:
        result = parse_top_level();
        break;
    }

    if (AST::is_err(result)) {
        shift_token();
    }

    return result;
}

bool Parser::reached_end(void) const {
    return cur_token == tok_eof;
}

}
