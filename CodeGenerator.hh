#pragma once

#include <memory>
#include <string>
#include <iostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "AST.hh"

namespace Kaleidoscope {

class CodeGenerator {

public:

    CodeGenerator(std::string name);

    llvm::Value *codegen_value(const AST::NumberLiteral &);
    llvm::Value *codegen_value(const AST::VariableName &);
    llvm::Value *codegen_value(const AST::BinaryOp &);
    llvm::Value *codegen_value(const AST::FunctionCall &);

    llvm::Function *codegen_func(const AST::FunctionPrototype &);
    llvm::Function *codegen_func(const AST::FunctionDefinition &);

    void emit(std::ostream &);

private:

    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::map<std::string, llvm::Value *> names;

};

}
