#pragma once

#include <memory>

#include "AST.hh"
#include "Lexer.hh"

namespace Kaleidoscope {

/**
 * @brief A parser parameterized on an input stream.
 */
class Parser {
private:
    int cur_token;
    Lexer lexer;

    int shift_token(void);
    int get_token_precedence(void) const;

    AST::NumberLiteral parse_number(void);
    AST::Expression parse_parens(void);
    AST::Expression parse_identifier(void);
    AST::Expression parse_binop_rhs(int prec, AST::Expression lhs);
    AST::Expression parse_if_then_else(void);
    AST::Expression parse_for_loop(void);
    AST::Expression parse_primary(void);
    AST::Expression parse_expression(void);

    std::unique_ptr<AST::FunctionPrototype> parse_prototype(void);
    AST::Declaration parse_definition(void);
    AST::Declaration parse_extern(void);

    AST::Declaration parse_top_level(void);

public:
    Parser(std::istream *input = &std::cin);

    /**
     * @brief Parse and return a top-level AST node.
     *
     * Returns `nullptr` and prints a message to std::cerr in case of an
     * error.  Returns `nullptr` in case of EOF.
     */
    AST::Declaration parse(void);

    /**
     * @brief Have we reached the end of the input stream?
     */
    bool reached_end(void) const;
};

}
