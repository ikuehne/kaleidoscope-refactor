/**
 * @brief Contains the various classes that make up the Abstract Syntax Tree.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/Value.h"

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

// Forward declare this for `host_generator` methods.
class CodeGenerator;

/**
 * @brief Contains all of the Abstract Syntax Tree classes.
 *
 * There are many small classes here, many with generic names likely to appear
 * elsewhere in the compiler.  We use a namespace to distinguish the AST.
 */
namespace AST {

/**
 * @brief Interface for expressions which evaluate to a Kaleidoscope value.
 */
class Expression {
public:
    virtual ~Expression() = default;

    /**
     * @brief Host the CodeGenerator visitor.
     *
     * All expressions correspond to an llvm::Value, so the generator should
     * produce one from this node.
     *
     * @return The generated `value`, essentially the LLVM AST version of this
     *         AST node.  The returned value is owned by the `CodeGenerator`
     *         parameter.
     */
    virtual llvm::Value *host_generator(CodeGenerator &) const = 0;
};

/**
 * @brief Floating-point literals.
 */
class NumberLiteral: public Expression {
public:
    NumberLiteral(double val);

    /**
     * @brief Get the value of this literal.
     */
    double get(void) const;

    llvm::Value *host_generator(CodeGenerator &) const override;

private:
    double val;
};

/**
 * @brief Variable names.
 *
 * Essentially a thin wrapper over `std::string`.
 */
class VariableName: public Expression {
public:
    VariableName(const std::string &);

    /**
     * @brief Get this variable's name as a `std::string`.
     */
    std::string get(void) const;

    llvm::Value *host_generator(CodeGenerator &) const override;

private:
    std::string name;
};

/**
 * @brief Binary operations of the form `expression op expression`.
 */
class BinaryOp: public Expression {
public:
    BinaryOp(char op, std::unique_ptr<Expression> lhs,
                      std::unique_ptr<Expression> rhs);
    char get_op(void) const;
    const Expression &get_lhs(void) const;
    const Expression &get_rhs(void) const;
    llvm::Value *host_generator(CodeGenerator &) const override;

private:
    char op;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

/**
 * @brief A call to a Kaleidoscope function.
 */
class FunctionCall: public Expression {
public:
    FunctionCall(const std::string &name,
                 std::vector<std::unique_ptr<Expression>> args);
    std::string get_name(void) const;
    const std::vector<std::unique_ptr<Expression>> &get_args(void) const;
    llvm::Value *host_generator(CodeGenerator &) const;

private:
    std::string fname;
    std::vector<std::unique_ptr<Expression>> args;
};

/**
 * @brief Top-level AST nodes, which can serve as the root of an AST.
 */
class Toplevel {
public:
    virtual ~Toplevel() = default;

    /**
     * @brief Host the CodeGenerator visitor.
     *
     * All top-level nodes correspond to an llvm::Function, so the generator
     * should produce one from this node.
     *
     * @return The generated `Function`, essentially the LLVM AST version of
     *         this AST node.  The returned value is owned by the
     *         `CodeGenerator` parameter.
     */
    virtual llvm::Function *host_generator(CodeGenerator &) const = 0;
};

/**
 * @brief Kaleidoscope function signature.
 */
class FunctionPrototype: public Toplevel {
public:
    FunctionPrototype(const std::string &name, std::vector<std::string> args);
    std::string get_name(void) const;
    std::vector<std::string> get_args(void) const;
    llvm::Function *host_generator(CodeGenerator &) const override;

private:
    std::string fname;
    std::vector<std::string> args;
};

/**
 * @brief A full function definition (signature and body).
 */
class FunctionDefinition: public Toplevel {
public:
    FunctionDefinition(std::unique_ptr<FunctionPrototype> proto,
                       std::unique_ptr<Expression> body);

    const FunctionPrototype &get_prototype(void) const;
    const Expression &get_body(void) const;
    llvm::Function *host_generator(CodeGenerator &) const override;

private:
    std::unique_ptr<FunctionPrototype> proto;
    std::unique_ptr<Expression> body;
};

}

}
