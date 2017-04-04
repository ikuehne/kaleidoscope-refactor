#pragma once

#include <memory>
#include <string>
#include <iostream>

#include <boost/variant.hpp>
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Target/TargetMachine.h"

#include "AST.hh"

namespace Kaleidoscope {

/**
 * @brief Visitor for AST nodes that evaluate directly to llvm::Values.
 */
class ExpressionGenerator: public boost::static_visitor<llvm::Value *> {
public:
    /**
     * @brief Get an ExpressionGenerator with references to the given
     *        CodeGeneratorImpl resources.
     *
     * ExpressionGenerators are intended *only* to live within
     * CodeGeneratorImpls; if it were not for the restrictions of
     * boost::variant (namely that the visitor method has to be operator(), so
     * it can only have one return type) this class's functionality would be
     * within ExpressionGenerator.  As the next best thing, it contains
     * pointers to CodeGeneratorImpl's internal state.  A bit gross, but as
     * long as ExpressionGenerator is used only as intended (and *not* exposed
     * to the outside world) it is no problem.
     */
    ExpressionGenerator(llvm::LLVMContext &context,
                        llvm::IRBuilder<> &builder,
                        llvm::Module &module,
                        std::map<std::string, llvm::AllocaInst *> &names) 
        : context(context), builder(builder), module(module), names(names) {}

    /**
     * @name Visitors
     *
     * Methods for visiting AST nodes.
     */
    /**@{*/

    llvm::Value *operator() (const AST::NumberLiteral &);
    llvm::Value *operator() (const AST::VariableName &);
    llvm::Value *operator() (const std::unique_ptr<AST::BinaryOp> &);
    llvm::Value *operator() (const std::unique_ptr<AST::FunctionCall> &);
    llvm::Value *operator() (const std::unique_ptr<AST::IfThenElse> &);
    llvm::Value *operator() (const std::unique_ptr<AST::ForLoop> &);
    llvm::Value *operator() (const std::unique_ptr<AST::LocalVar> &);

    /**@}*/

private:
    llvm::Value *to_cond(llvm::Value *f);

    llvm::LLVMContext &context;
    llvm::IRBuilder<> &builder;
    llvm::Module &module;
    std::map<std::string, llvm::AllocaInst *> &names;
};

/**
 * @brief Visit AST nodes and convert them to an LLVM AST.
 */
class CodeGeneratorImpl {

public:

    /* See CodeGenerator.hh for documentation on these methods.  CodeGenerator
     * exposes thin wrappers over them. */
    CodeGeneratorImpl(std::string name, std::string triple);
    llvm::Function *operator()
        (const std::unique_ptr<AST::FunctionPrototype> &);
    llvm::Function *operator()
        (const std::unique_ptr<AST::FunctionDefinition> &);
    llvm::Function *operator()(const AST::Error &) {
        return nullptr;
    }

    void run_passes(void);

    void emit_ir(std::ostream &);
    void emit_obj(int fd);

private:

    llvm::LLVMContext context;

    /**
     * @brief LLVM's helper for emitting IR.
     */
    llvm::IRBuilder<> builder;

    /**
     * @brief The module we are constructing.
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * @brief Current namespace.
     */
    std::map<std::string, llvm::AllocaInst *> names;

    /**
     * @brief The target machine (target triple + CPU information).
     */
	llvm::TargetMachine *target;

    /**
     * @brief A visitor for value nodes; see above.
     */
    ExpressionGenerator expr_gen;
};

}
