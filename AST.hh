#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Kaleidoscope {

namespace AST {

class Expression {
public:
    virtual ~Expression() = default;
};

class NumberLiteral: public Expression {
public:
    NumberLiteral(double val);
    double get(void) const;
private:
    double val;
};

class VariableName: public Expression {
public:
    VariableName(const std::string &Name);
    std::string get(void) const;

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

private:
    std::string fname;
    std::vector<std::unique_ptr<Expression>> args;
};

class FunctionPrototype {
public:
    FunctionPrototype(const std::string &name, std::vector<std::string> args);
    std::string get_name(void) const;
    std::vector<std::string> get_args(void) const;

private:
    std::string fname;
    std::vector<std::string> args;
};

class FunctionDefinition {
public:
    FunctionDefinition(std::unique_ptr<FunctionPrototype> proto,
                       std::unique_ptr<Expression> body);

    const FunctionPrototype &get_prototype(void) const;
    const Expression &get_body(void) const;

private:
    std::unique_ptr<FunctionPrototype> proto;
    std::unique_ptr<Expression> body;
};

}

}
