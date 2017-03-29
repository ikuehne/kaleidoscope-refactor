#pragma once

#include <memory>
#include <string>
#include <iostream>

#include <boost/variant.hpp>
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Host.h"
#include "llvm/Target/TargetMachine.h"

#include "AST.hh"

namespace Kaleidoscope {

class ExpressionGenerator: public boost::static_visitor<llvm::Value *> {
public:
    ExpressionGenerator(llvm::LLVMContext &context,
                        llvm::IRBuilder<> &builder,
                        llvm::Module &module,
                        std::map<std::string, llvm::Value *> &names) 
        : context(context), builder(builder), module(module), names(names) {}

    llvm::Value *operator() (const AST::NumberLiteral &);
    llvm::Value *operator() (const AST::VariableName &);
    llvm::Value *operator() (const std::unique_ptr<AST::BinaryOp> &);
    llvm::Value *operator() (const std::unique_ptr<AST::FunctionCall> &);
    llvm::Value *operator() (const AST::Error &) {
        return nullptr;
    }

private:
    llvm::LLVMContext &context;
    llvm::IRBuilder<> &builder;
    llvm::Module &module;
    std::map<std::string, llvm::Value *> &names;
};

/**
 * @brief Visit AST nodes and convert them to an LLVM AST.
 */
class CodeGenerator {

public:
    /**
     * @brief Create a `CodeGenerator` appending definitions to a module with
     *        the given name.
     */
    CodeGenerator(std::string name,
                  std::string triple=llvm::sys::getDefaultTargetTriple());

    /**
     * @name Visitors
     *
     * Methods for visiting AST nodes.
     */
    /**@{*/

    llvm::Function *operator()
        (const std::unique_ptr<AST::FunctionPrototype> &);
    llvm::Function *operator()
        (const std::unique_ptr<AST::FunctionDefinition> &);
    llvm::Function *operator()(const AST::Error &) {
        return nullptr;
    }

    /**@}*/

    /**
     * @brief Emit LLVM IR to the given output stream.
     */
    void emit_ir(std::ostream &);

    /**
     * @brief Emit object code to the given output stream.
     *
     * @param fd A file descriptor to an open, readable file.  Will not be
     *           closed upon completion.
     */
    void emit_obj(int fd);

private:

    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::map<std::string, llvm::Value *> names;

	llvm::TargetMachine *target;

    ExpressionGenerator expr_gen;
};

}
