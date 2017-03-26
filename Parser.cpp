#include "Parser.hh"

#include <iostream>
#include <map>
#include <vector>

namespace Kaleidoscope {

/// BinopPrecedence - This holds the precedence for each binary operator that
// is defined.
static const std::map<char, int> BINOP_PRECEDENCE {
    {'<', 10},
    {'+', 20},
    {'-', 20},
    {'*', 40},
};

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int Parser::get_token_precedence(void) const {
    if (!isascii(cur_token))
        return -1;

    if (BINOP_PRECEDENCE.count(cur_token) == 0) return -1;

    // Make sure it's a declared binop.
    int result = BINOP_PRECEDENCE.find(cur_token)->second;

    return result;
}

static std::unique_ptr<AST::Expression> log_error(const char *str) {
    std::cerr << "Kaleidoscope::Parser::log_error: "
              << str << std::endl;
    return nullptr;
}

static std::unique_ptr<AST::FunctionPrototype> log_error_p(const char *s) {
    log_error(s);
    return nullptr;
}

Parser::Parser(std::istream *input) {
    lexer = Lexer(input);
    shift_token();
}

int Parser::shift_token(void) {
    cur_token = lexer.get_token();
    return cur_token;
}

std::unique_ptr<AST::Expression> Parser::parse_number(void) {
    auto result = std::make_unique<AST::NumberLiteral>(lexer.get_number());
    /* Advance the lexer. */
    shift_token();
    return std::move(result);
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<AST::Expression> Parser::parse_parens(void) {
  	shift_token();
    auto contents = parse_expression();
    if (!contents)
        return nullptr;

    if (cur_token != ')')
        return log_error("expected ')'");

    // Eat the closing parens.
    shift_token();
    return contents;
}

std::unique_ptr<AST::Expression> Parser::parse_identifier(void) {
    std::string id = lexer.get_identifier();
    shift_token();    // eat identifier.

    if (cur_token != '(') // Simple variable ref.
        return std::make_unique<AST::VariableName>(id);

    // Call.
    shift_token();    // eat (
    std::vector<std::unique_ptr<AST::Expression>> args;
    if (cur_token != ')') {
        while (1) {
            if (auto arg = parse_expression())
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (cur_token == ')')
                break;

            if (cur_token != ',')
                return log_error("Expected ')' or ',' in argument list");
            shift_token();
        }
    }

    // Eat the ')'.
    shift_token();

    return std::make_unique<AST::FunctionCall>(lexer.get_identifier(),
                                               std::move(args));
}

std::unique_ptr<AST::Expression> Parser::parse_primary(void) {
    switch (cur_token) {
        case tok_identifier:
            return parse_identifier();
        case tok_number:
            return parse_number();
        case '(':
            return parse_parens();
        default:
            return log_error("Unknown token when expecting expression.");
    }
}

std::unique_ptr<AST::Expression> Parser::parse_expression(void) {
    auto lhs = parse_primary();
    if (!lhs) return nullptr;
    return parse_binop_rhs(0, std::move(lhs));
}

std::unique_ptr<AST::Expression>
    Parser::parse_binop_rhs(int prec, std::unique_ptr<AST::Expression> lhs) {

    while (true) {
        int op_prec = get_token_precedence();
        /* If the precedence on the right is lower than on the left, return
         * the expression on the left. */
        if (op_prec < prec) return lhs;
        int op = cur_token;
        shift_token();
        auto rhs = parse_primary();
        if (!rhs) return nullptr;
        int next_prec = get_token_precedence();
        if (op_prec < next_prec) {
            rhs = parse_binop_rhs(op_prec + 1, std::move(rhs));

            if (!rhs) return nullptr;
        }

        lhs = std::make_unique<AST::BinaryOp>(op, std::move(lhs),
                                                  std::move(rhs));
    }
}

std::unique_ptr<AST::FunctionPrototype> Parser::parse_prototype(void) {
    if (cur_token != tok_identifier)
        return log_error_p("Expected function name in prototype");

    std::string fname = lexer.get_identifier();
    shift_token();

    if (cur_token != '(')
        return log_error_p("Expected '(' in prototype");

    // Read the list of argument names.
    std::vector<std::string> args;
    while (shift_token() == tok_identifier)
        args.push_back(lexer.get_identifier());
    if (cur_token != ')')
        return log_error_p("Expected ')' in prototype");

    // success.
    shift_token();    // eat ')'.

    return std::make_unique<AST::FunctionPrototype>(fname, std::move(args));
}

std::unique_ptr<AST::FunctionDefinition> Parser::parse_definition(void) {
    shift_token();  // eat def.
    auto proto = parse_prototype();
    if (!proto) return nullptr;

    if (auto expr = parse_expression())
        return std::make_unique<AST::FunctionDefinition>(std::move(proto),
                                                         std::move(expr));

    return nullptr;
}

std::unique_ptr<AST::FunctionPrototype> Parser::parse_extern(void) {
    shift_token();
    return parse_prototype();
}

std::unique_ptr<AST::FunctionDefinition> Parser::parse_top_level(void) {
    if (auto expr = parse_expression()) {
        // Make an anonymous proto.
        auto proto = std::make_unique<AST::FunctionPrototype>(
			   "", std::vector<std::string>());
        return std::make_unique<AST::FunctionDefinition>(std::move(proto),
														 std::move(expr));
    }
    return nullptr; 
}

std::unique_ptr<AST::Toplevel> Parser::parse(void) {
    std::unique_ptr<AST::Toplevel> result;
    switch (cur_token) {
    case tok_eof:
        return nullptr;
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

    if (!result) shift_token();

    return result;
}

void Parser::handle_definition(void) {
    if (parse_definition()) {
    } else {
        // Skip token for error recovery.
        shift_token();
    }
}

void Parser::handle_extern(void) {
    if (parse_extern()) {
    } else {
        // Skip token for error recovery.
        shift_token();
    }
}

void Parser::handle_top_level(void) {
    // Evaluate a top-level expression into an anonymous function.
    if (parse_top_level()) {
    } else {
        // Skip token for error recovery.
        shift_token();
    }
}

/// top ::= definition | external | expression | ';'
void Parser::main_loop() {
    while (true) {
        std::cerr << "ready> ";
        switch (cur_token) {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            shift_token();
            break;
        case tok_def:
            handle_definition();
            break;
        case tok_extern:
            handle_extern();
            break;
        default:
            handle_top_level();
            break;
        }
    }
}

void Parser::demo(void) {
    std::cerr << "ready> ";
    main_loop();
}

bool Parser::reached_end(void) const {
    return cur_token == tok_eof;
}

}
