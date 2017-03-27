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

    std::unique_ptr<AST::Expression>
        parse_binop_rhs(int prec, std::unique_ptr<AST::Expression> lhs);
    void handle_definition(void);
    void handle_extern(void);
    void handle_top_level(void);
    void main_loop(void);

    std::unique_ptr<AST::Expression> parse_number(void);
    std::unique_ptr<AST::Expression> parse_parens(void);
    std::unique_ptr<AST::Expression> parse_identifier(void);
    std::unique_ptr<AST::Expression> parse_primary(void);
    std::unique_ptr<AST::Expression> parse_expression(void);

    std::unique_ptr<AST::FunctionPrototype> parse_prototype(void);
    std::unique_ptr<AST::FunctionDefinition> parse_definition(void);
    std::unique_ptr<AST::FunctionPrototype> parse_extern(void);

    std::unique_ptr<AST::FunctionDefinition> parse_top_level(void);

public:
    Parser(std::istream *input = &std::cin);

    /**
     * @brief Parse and return a top-level AST node.
     *
     * Returns `nullptr` and prints a message to std::cerr in case of an
     * error.  Returns `nullptr` in case of EOF.
     */
    std::unique_ptr<AST::Toplevel> parse(void);

    /**
     * @brief Have we reached the end of the input stream?
     */
    bool reached_end(void) const;
    void demo(void);
};

}
