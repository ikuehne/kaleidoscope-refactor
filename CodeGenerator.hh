#pragma once

#include <memory>
#include <string>
#include <iostream>

#include "llvm/ADT/Triple.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Host.h"
#include "llvm/Target/TargetMachine.h"

#include "AST.hh"

namespace Kaleidoscope {

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

    llvm::Value *codegen_value(const AST::NumberLiteral &);
    llvm::Value *codegen_value(const AST::VariableName &);
    llvm::Value *codegen_value(const AST::BinaryOp &);
    llvm::Value *codegen_value(const AST::FunctionCall &);

    llvm::Function *codegen_func(const AST::FunctionPrototype &);
    llvm::Function *codegen_func(const AST::FunctionDefinition &);

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
};

}
