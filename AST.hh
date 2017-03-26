#pragma once

#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/Value.h"

namespace Kaleidoscope {

class CodeGenerator;

namespace AST {

class Expression {
public:
    virtual ~Expression() = default;
    virtual llvm::Value *host_generator(CodeGenerator &cg) const = 0;
};

class NumberLiteral: public Expression {
public:
    NumberLiteral(double val);
    double get(void) const;
    llvm::Value *host_generator(CodeGenerator &cg) const override;

private:
    double val;
};

class VariableName: public Expression {
public:
    VariableName(const std::string &Name);
    std::string get(void) const;
    llvm::Value *host_generator(CodeGenerator &cg) const override;

private:
    std::string name;
};

class BinaryOp: public Expression {
public:
    BinaryOp(char op, std::unique_ptr<Expression> lhs,
                      std::unique_ptr<Expression> rhs);
    char get_op(void) const;
    const Expression &get_lhs(void) const;
    const Expression &get_rhs(void) const;
    llvm::Value *host_generator(CodeGenerator &cg) const override;

private:
    char op;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

class FunctionCall: public Expression {
public:
    FunctionCall(const std::string &name,
                 std::vector<std::unique_ptr<Expression>> args);
    std::string get_name(void) const;
    const std::vector<std::unique_ptr<Expression>> &get_args(void) const;
    llvm::Value *host_generator(CodeGenerator &cg) const;

private:
    std::string fname;
    std::vector<std::unique_ptr<Expression>> args;
};

class Toplevel {
public:
    virtual ~Toplevel() = default;
    virtual llvm::Function *host_generator(CodeGenerator &cg) const = 0;
};

class FunctionPrototype: public Toplevel {
public:
    FunctionPrototype(const std::string &name, std::vector<std::string> args);
    std::string get_name(void) const;
    std::vector<std::string> get_args(void) const;
    llvm::Function *host_generator(CodeGenerator &cg) const override;

private:
    std::string fname;
    std::vector<std::string> args;
};

class FunctionDefinition: public Toplevel {
public:
    FunctionDefinition(std::unique_ptr<FunctionPrototype> proto,
                       std::unique_ptr<Expression> body);

    const FunctionPrototype &get_prototype(void) const;
    const Expression &get_body(void) const;
    llvm::Function *host_generator(CodeGenerator &cg) const override;

private:
    std::unique_ptr<FunctionPrototype> proto;
    std::unique_ptr<Expression> body;
};

}

}
