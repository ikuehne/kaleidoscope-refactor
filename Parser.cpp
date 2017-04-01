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

static ErrorInfo merge(ErrorInfo start, ErrorInfo end) {
    return ErrorInfo(start.filename,
                     start.charno_start, start.lineno_start,
                     end.charno_end,     end.lineno_end);
}

[[noreturn]] static void _throw(std::string msg, ErrorInfo annotation) {
    throw Error("Parser error", msg, annotation);
}

/* Look up the precedence of the current token. */
int Parser::get_token_precedence(void) const {
    if (!isascii(cur_token.second))
        return -1;

    /* Can't just use operator[] because `BINOP_PRECEDENCE` is `const`. */
    if (BINOP_PRECEDENCE.count(cur_token.second) == 0) return -1;

    // Make sure it's a declared binop.
    int result = BINOP_PRECEDENCE.find(cur_token.second)->second;

    return result;
}

Parser::Parser(std::string input)
    : lexer(input), cur_token(ErrorInfo(nullptr, 0, 0, 0, 0), 0) {
    shift_token();
}

int Parser::shift_token(void) {
    /* `cur_token` gives us 1 token of lookahead. */
    cur_token = lexer.get_token();
    return cur_token.second;
}

AST::Expression Parser::parse_number(void) {
    AST::NumberLiteral result(lexer.get_number(), cur_token.first);
    /* Advance the lexer. */
    shift_token();
    return result;
}

AST::Expression Parser::parse_parens(void) {
    auto start = cur_token.first;
    /* Shift the opening paren. */
  	shift_token();
    /* Get the body of the expression. */
    auto contents = parse_expression();

    /* If it didn't end in a close paren, error. */
    if (cur_token.second != ')')  {
        _throw("expected ')'", merge(start, cur_token.first));
    }

    /* Shift the closing paren. */
    shift_token();
    return contents;
}

AST::Expression Parser::parse_identifier(void) {

    auto start = cur_token.first;
    /* Get the identifier. */
    std::string id = lexer.get_identifier();

    /* Shift the identifier. */
    shift_token();

    /* Unless this is a function call, */
    if (cur_token.second != '(') {
        /* it's a variable. */
        return AST::VariableName(id, merge(start, cur_token.first));
    }

    /* If it is a function call, shift the opening paren.*/
    shift_token();
    /* Collect an argument vector. */
    std::vector<AST::Expression> args;
    /* (unless there are no arguments). */
    if (cur_token.second != ')') {
        while (1) {
            AST::Expression arg(parse_expression());
            args.push_back(std::move(arg));

            if (cur_token.second == ')') break;

            if (cur_token.second != ',') {
                _throw("expected ')' or ',' in argument list",
                       merge(start, cur_token.first));
            }

            shift_token();
        }
    }

    // Eat the ')'.
    shift_token();

    return std::make_unique<AST::FunctionCall>(
            id, std::move(args), merge(start, cur_token.first));
}

AST::Expression Parser::parse_primary(void) {
    switch (cur_token.second) {
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
            _throw("unkown token when expecting expression", cur_token.first);
    }
}

AST::Expression Parser::parse_expression(void) {
    auto lhs = parse_primary();
    return parse_binop_rhs(0, std::move(lhs));
}

AST::Expression Parser::parse_binop_rhs(int prec, AST::Expression lhs) {
    auto start = cur_token.first;
    while (true) {
        int op_prec = get_token_precedence();

        /* If the precedence on the right is lower than on the left, return
         * the expression on the left. */
        if (op_prec < prec) return lhs;

        /* Save and shift the current token. */
        int op = cur_token.second;
        shift_token();

        /* Parse the right-hand side. */
        auto rhs = parse_primary();

        /* Check the precedence of the next operator.  (Note that if the next
         * token is *not* an operator, `get_token_precedence` will return -1).
        */
        int next_prec = get_token_precedence();
        if (op_prec < next_prec) {
            rhs = parse_binop_rhs(op_prec + 1, std::move(rhs));
        }

        auto info = merge(AST::get_info(lhs), AST::get_info(rhs));
        lhs = std::make_unique<AST::BinaryOp>(
                op, std::move(lhs), std::move(rhs), info);
    }
}

AST::Expression Parser::parse_if_then_else(void){
    auto start = cur_token.first;
    /* Shift "if". */
    shift_token();

    auto cond = parse_expression();
    if (cur_token.second != tok_then) {
        _throw("expected \"then\"", merge(start, cur_token.first));
    }

    /* Shift "then". */
    shift_token();
    auto then = parse_expression();

    if (cur_token.second != tok_else) {
        _throw("expected \"else\"", merge(start, cur_token.first));
    }

    /* Shift "else". */
    shift_token();
    auto _else = parse_expression();

    return std::make_unique<AST::IfThenElse>(
                std::move(cond), std::move(then),
                std::move(_else), merge(start, cur_token.first));
}

AST::Expression Parser::parse_for_loop(void) {
    auto start = cur_token.first;
    /* Shift "for". */
    shift_token();

    if (cur_token.second != tok_identifier) {
        _throw("expected identifier as loop index",
               merge(start, cur_token.first));
    }

    std::string idx = lexer.get_identifier();

    shift_token();

    if (cur_token.second != '=') {
        _throw("expected '=' in loop", merge(start, cur_token.first));
    }

    shift_token();

    auto init = parse_expression();

    if (cur_token.second != ',') {
        _throw("expected ',' between loop elements",
               merge(start, cur_token.first));
    }

    shift_token();

    auto term = parse_expression();

    AST::Expression incr(AST::NumberLiteral(1.0, cur_token.first));
    if (cur_token.second == ',') {
        shift_token();
        incr = parse_expression();
    }

    if (cur_token.second != tok_in) {
        _throw("expected \"in\" after for loop",
               merge(start, cur_token.first));
    }

    shift_token();

    auto body = parse_expression();

    return std::make_unique<AST::ForLoop>(
            idx, std::move(init), std::move(term),
            std::move(incr), std::move(body), merge(start, cur_token.first));
}

std::unique_ptr<AST::FunctionPrototype> Parser::parse_prototype(void) {
    auto start = cur_token.first;
    if (cur_token.second != tok_identifier) {
        _throw("expected function name in prototype", start);
    }

    std::string fname = lexer.get_identifier();
    shift_token();

    if (cur_token.second != '(') {
        _throw("expected '(' in prototype", cur_token.first);
    }

    /* Read the list of argument names. */
    std::vector<std::string> args;
    while (shift_token() == tok_identifier)
        args.push_back(lexer.get_identifier());
    if (cur_token.second != ')') {
        _throw("expected ')' in prototype", cur_token.first);
    }

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

    /* Turn it into the body of an anonymous prototype. */
    auto proto =
        std::make_unique<AST::FunctionPrototype>("",
                                                 std::vector<std::string>());
    return std::make_unique<AST::FunctionDefinition>(
            AST::FunctionDefinition(std::move(proto), std::move(expr)));
}

AST::Declaration Parser::parse(void) {
    AST::Declaration result;
    try {
        switch (cur_token.second) {
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
    } catch (Error) {
        shift_token();
        throw;
    }

    return result;
}

bool Parser::reached_end(void) const {
    return cur_token.second == tok_eof;
}

}
