#pragma once

#include <memory>

#include "AST.hh"
#include "Lexer.hh"

namespace Kaleidoscope {

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

public:
    Parser(std::istream *input = &std::cin);

    std::unique_ptr<AST::Expression> parse_number(void);
    std::unique_ptr<AST::Expression> parse_parens(void);
    std::unique_ptr<AST::Expression> parse_identifier(void);
    std::unique_ptr<AST::Expression> parse_primary(void);
    std::unique_ptr<AST::Expression> parse_expression(void);

    std::unique_ptr<AST::FunctionPrototype> parse_prototype(void);
    std::unique_ptr<AST::FunctionDefinition> parse_definition(void);
    std::unique_ptr<AST::FunctionPrototype> parse_extern(void);

    std::unique_ptr<AST::FunctionDefinition> parse_top_level(void);
    void demo(void);
};

}
