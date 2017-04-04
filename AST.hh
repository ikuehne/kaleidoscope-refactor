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

#include "Error.hh"

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

struct Error { };

/**
 * @brief Floating-point literals.
 */
struct NumberLiteral {
    double val;
    ErrorInfo info;
    NumberLiteral(double val, ErrorInfo info): val(val), info(info) {}
};

/**
 * @brief Variable names.
 *
 * Essentially a thin wrapper over `std::string`.
 */
struct VariableName {
    std::string name;
    ErrorInfo info;
    VariableName(std::string name, ErrorInfo info): name(name), info(info) {}
};

struct BinaryOp;
struct FunctionCall;
struct IfThenElse;
struct ForLoop;
struct LocalVar;

/**
 * @brief An expression: any of the various expression structs.
 */
typedef boost::variant< NumberLiteral,
                        VariableName,
                        std::unique_ptr<BinaryOp>,
                        std::unique_ptr<FunctionCall>,
                        std::unique_ptr<IfThenElse>,
                        std::unique_ptr<ForLoop>,
                        std::unique_ptr<LocalVar> > Expression;

ErrorInfo get_info(const Expression &);

/**
 * @brief Binary operations of the form `expression op expression`.
 */
struct BinaryOp {
    char op;
    Expression lhs;
    Expression rhs;
    ErrorInfo info;
    BinaryOp(char op, Expression lhs, Expression rhs, ErrorInfo info)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), info(info) {}
};

/**
 * @brief A call to a Kaleidoscope function.
 */
struct FunctionCall {
    std::string fname;
    std::vector<Expression> args;
    ErrorInfo info;
    FunctionCall(std::string fname,
                 std::vector<Expression> args,
                 ErrorInfo info)
        : fname(fname), args(std::move(args)), info(info) {}
};

/**
 * @brief An if/then/else expression.
 */
struct IfThenElse {
    Expression cond, then, else_;
    ErrorInfo info;

    IfThenElse(Expression cond,  Expression then,
               Expression else_, ErrorInfo info)
        : cond(std::move(cond)),  then(std::move(then)),
          else_(std::move(else_)), info(info) {}
};

/**
 * @brief A "for" loop.
 */
struct ForLoop {
    std::string index_var;
    Expression start, end, step, body;
    ErrorInfo info;

    ForLoop(std::string index_var,
            Expression start, Expression end,
            Expression step,  Expression body,
            ErrorInfo info)
        : index_var(index_var),
          start(std::move(start)), end(std::move(end)),
          step(std::move(step)),   body(std::move(body)),
          info(info) {}
};

struct LocalVar {
    std::vector<std::pair<std::string, Expression>> names;
    Expression body;
    ErrorInfo info;
    LocalVar(std::vector<std::pair<std::string, Expression>> names,
             Expression body, ErrorInfo info)
        : names(std::move(names)), body(std::move(body)), info(info) {}
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
