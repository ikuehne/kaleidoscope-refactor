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

/* A note on the architecture: AST classes are returned by the parser, and
 * then visited by the code-generator. The classes contained herein have the
 * following in common:
 *
 * - They represent a syntactic construct in Kaleidoscope.
 * - They contain only `const` methods.  The AST should not be modified.
 * - Instances can host a `CodeGenerator` visitor to generate code from their
 *   contents.
 */

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

/* Forward-declare theses to prevent recursive type. */
struct BinaryOp;
struct FunctionCall;

/**
 * @brief An expression: any of the various expression structs.
 */
typedef boost::variant< NumberLiteral,
                        VariableName,
                        std::unique_ptr<BinaryOp>,
                        std::unique_ptr<FunctionCall>,
                        Error > Expression;

inline bool is_err(const Expression &expr) {
    return expr.which() == 4;
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

struct FunctionPrototype;
struct FunctionDefinition;

typedef boost::variant< std::unique_ptr< FunctionPrototype >,
                        std::unique_ptr< FunctionDefinition >,
                        Error > Declaration;

#define is_err_decl(d) (d.which() == 2)

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
