#include "AST.hh"
#include "CodeGenerator.hh"

#define HOST_OVERRIDE(_class) llvm::Value *\
    _class::host_generator(CodeGenerator &cg) const {\
        return cg.codegen_value(*this);\
    }

namespace Kaleidoscope {
namespace AST {

/*****************************************************************************
 * NumberLiteral implementations.
 */

NumberLiteral::NumberLiteral(double val): val(val) {};

double NumberLiteral::get(void) const {
    return val;
}

HOST_OVERRIDE(NumberLiteral)

/*****************************************************************************
 * VariableName implementations.
 */

VariableName::VariableName(const std::string &name): name(name) {}

std::string VariableName::get(void) const {
    return name;
}

HOST_OVERRIDE(VariableName)

/*****************************************************************************
 * BinaryOp implementations.
 */

BinaryOp::BinaryOp(char op, std::unique_ptr<Expression> lhs,
                            std::unique_ptr<Expression> rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

char BinaryOp::get_op(void) const { return op; }

const Expression &BinaryOp::get_lhs(void) const {
    return *lhs;
}

const Expression &BinaryOp::get_rhs(void) const {
    return *rhs;
}

HOST_OVERRIDE(BinaryOp)

/*****************************************************************************
 * FunctionCall implementations.
 */

FunctionCall::FunctionCall(const std::string &name,
                           std::vector<std::unique_ptr<Expression>> args)
    : fname(name), args(std::move(args)) {}

std::string FunctionCall::get_name(void) const { return fname; }

const std::vector<std::unique_ptr<Expression>> &
      FunctionCall::get_args(void) const {
    return args;
}

llvm::Value *FunctionCall::host_generator(CodeGenerator &cg) const {
    return cg.codegen_value(*this);
}

/*****************************************************************************
 * FunctionPrototype implementations.
 */

FunctionPrototype::FunctionPrototype(const std::string &name,
                                     std::vector<std::string> args)
    : fname(name), args(args) {}

std::string FunctionPrototype::get_name(void) const {
    return fname;
}

std::vector<std::string> FunctionPrototype::get_args(void) const {
    return args;
}

llvm::Function *FunctionPrototype::host_generator(CodeGenerator &cg) const {
    return cg.codegen_func(*this);
}

/*****************************************************************************
 * FunctionDefinition implementations.
 */

FunctionDefinition::FunctionDefinition(
        std::unique_ptr<FunctionPrototype> proto,
        std::unique_ptr<Expression> body)
    : proto(std::move(proto)), body(std::move(body)) {}

const FunctionPrototype &FunctionDefinition::get_prototype(void) const {
    return *proto;
}

const Expression &FunctionDefinition::get_body(void) const {
    return *body;
}

llvm::Function *FunctionDefinition::host_generator(CodeGenerator &cg) const {
    return cg.codegen_func(*this);
}

}

}
