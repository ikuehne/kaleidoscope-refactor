/**
 * @brief Contains the various classes that make up the Abstract Syntax Tree.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <boost/variant.hpp>
#include <llvm/IR/Value.h>

/* Architectural note: AST classes are returned by the parser, and then
 * visited by the code-generator. */

namespace Kaleidoscope {

/**
 * @brief Contains all of the Abstract Syntax Tree classes.
 *
 * There are many small classes here, many with generic names likely to appear
 * elsewhere in the compiler.  We use a namespace to distinguish the AST.
 */
namespace AST {

struct Error {

};

/**
 * @brief Floating-point literals.
 */
struct NumberLiteral {
    double val;

    NumberLiteral(double val): val(val) {}
};

/**
 * @brief Variable names.
 *
 * Essentially a thin wrapper over `std::string`.
 */
struct VariableName {
    std::string name;
    VariableName(std::string name): name(name) {}
};

/* Forward-declare these to prevent recursive type. */
struct BinaryOp;
struct FunctionCall;
struct IfThenElse;
struct ForLoop;

/**
 * @brief An expression: any of the various expression structs.
 */
typedef boost::variant< NumberLiteral,
                        VariableName,
                        std::unique_ptr<BinaryOp>,
                        std::unique_ptr<FunctionCall>,
                        std::unique_ptr<IfThenElse>,
                        std::unique_ptr<ForLoop>,
                        Error > Expression;

inline bool is_err(const Expression &expr) {
    return expr.which() == 6;
}

/**
 * @brief Binary operations of the form `expression op expression`.
 */
struct BinaryOp {
    char op;
    Expression lhs;
    Expression rhs;
    BinaryOp(char op, Expression lhs,
                      Expression rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

/**
 * @brief A call to a Kaleidoscope function.
 */
struct FunctionCall {
    std::string fname;
    std::vector<Expression> args;
    FunctionCall(std::string fname,
                 std::vector<Expression> args)
        : fname(fname), args(std::move(args)) {}
};

/**
 * @brief An if/then/else expression.
 */
struct IfThenElse {
    Expression cond, then, else_;

    IfThenElse(Expression cond, Expression then, Expression else_)
        : cond(std::move(cond)),
          then(std::move(then)),
          else_(std::move(else_)) {}
};

/**
 * @brief A "for" loop.
 */
struct ForLoop {
    std::string index_var;
    Expression start, end, step, body;

    ForLoop(std::string index_var,
            Expression start, Expression end,
            Expression step,  Expression body)
        : index_var(index_var),
                    start(std::move(start)), end(std::move(end)),
                    step(std::move(step)),   body(std::move(body)) {}
};

struct FunctionPrototype;
struct FunctionDefinition;

typedef boost::variant< std::unique_ptr< FunctionPrototype >,
                        std::unique_ptr< FunctionDefinition >,
                        Error > Declaration;

inline bool is_err(const Declaration &decl) {
    return decl.which() == 2;
}

/**
 * @brief Kaleidoscope function signature.
 */
struct FunctionPrototype {
    std::string fname;
    std::vector<std::string> args;
    FunctionPrototype(std::string fname, std::vector<std::string> args)
        : fname(fname), args(args) {}
};

/**
 * @brief A full function definition (signature and body).
 */
struct FunctionDefinition {
    std::unique_ptr<FunctionPrototype> proto;
    Expression body;
    FunctionDefinition(std::unique_ptr<FunctionPrototype> proto,
                       Expression body)
        : proto(std::move(proto)), body(std::move(body)) {}
};

}

}
