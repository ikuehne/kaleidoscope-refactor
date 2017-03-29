#include <iostream>
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"

#include "CodeGenerator.hh"

namespace Kaleidoscope {

static llvm::Value *log_error(const char *str) {
    std::cerr << "Kaleidoscope::CodeGenerator::log_error: "
              << str << std::endl;
    return nullptr;
}

llvm::Value *ExpressionGenerator::operator()(const AST::NumberLiteral &num) {
    /* Create a floating-point constant with this value in this context. */
    return llvm::ConstantFP::get(context, llvm::APFloat(num.val));
}

llvm::Value *ExpressionGenerator::operator()(const AST::VariableName &var) {
    /* Just look up the value corresponding to this name, and return that. */
    auto result = names[var.name];
    if (!result) return log_error("Unknown variable name");
    return result;
}

llvm::Value *ExpressionGenerator::operator()
       (const std::unique_ptr<AST::BinaryOp> &op) {
    /* Get the LLVM values for left and right. */
    llvm::Value *l = boost::apply_visitor(*this, op->lhs);
    llvm::Value *r = boost::apply_visitor(*this, op->rhs);
    if (!l || !r) return nullptr;

    switch(op->op) {
    case '+':
        return builder.CreateFAdd(l, r, "addtmp");
    case '-':
        return builder.CreateFSub(l, r, "subtmp");
    case '*':
        return builder.CreateFMul(l, r, "multmp");
    case '/':
        return builder.CreateFDiv(l, r, "divtmp");
    case '<':
        l = builder.CreateFCmpULT(l, r, "cmptmp");
        /* Convert bool 0/1 to double 0.0 or 1.0 */
        return builder.CreateUIToFP(
                l, llvm::Type::getDoubleTy(context), "booltmp");
    default:
        return log_error("invalid binary operator.");
    }
}

llvm::Value *ExpressionGenerator::operator()(
        const std::unique_ptr<AST::FunctionCall> &call) {
	/* Look up the name in the global module table. */
    llvm::Function *llvm_func = module.getFunction(call->fname);
	if (!llvm_func)
		return log_error("Unknown function referenced");

    /* Log argument mismatch error. */
    if (llvm_func->arg_size() != call->args.size())
        return log_error("Incorrect # arguments passed");

    std::vector<llvm::Value *> llvm_args;
    for (unsigned i = 0; i != call->args.size(); ++i) {
        llvm_args.push_back(boost::apply_visitor(*this, call->args[i]));
        if (!llvm_args.back()) return nullptr;
    }

    return builder.CreateCall(llvm_func, llvm_args, "calltmp");
}

CodeGenerator::CodeGenerator(std::string name,
                             std::string triple)
    : builder(context),
      module(llvm::make_unique<llvm::Module>(name, context)),
      expr_gen(ExpressionGenerator(context, builder, *module, names)) {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

	std::string error;
	auto target_triple = llvm::Triple(triple);
	/* TODO: Allow target-specific information. */
    auto llvm_target =
        llvm::TargetRegistry::lookupTarget("", target_triple, error);

	// Print an error and exit if we couldn't find the requested target.
	// This generally occurs if we've forgotten to initialise the
	// TargetRegistry or we have a bogus target triple.

	/* TODO: fix this to properly handle the error. */
	if (!llvm_target) {
	    llvm::errs() << error;
	}

    /* TODO: give a more specific CPU. */
    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions options;
    auto reloc_model = llvm::Reloc::Model();
    
    target = llvm_target->createTargetMachine(triple, cpu, features,
                                              options, reloc_model);
    module->setDataLayout(target->createDataLayout());
    module->setTargetTriple(triple);
}

llvm::Function *CodeGenerator::operator()
       (const std::unique_ptr<AST::FunctionPrototype> &func) {
    std::vector<llvm::Type *> doubles(func->args.size(),
                                      llvm::Type::getDoubleTy(context));
    llvm::FunctionType *ft =
        llvm::FunctionType::get(llvm::Type::getDoubleTy(context),
                                doubles, false);

    llvm::Function *result =
            llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                   func->fname, module.get());
    unsigned i = 0;
    for (auto &arg: result->args())
        arg.setName(func->args[i++]);

    return result;
}

llvm::Function *CodeGenerator::operator()
            (const std::unique_ptr<AST::FunctionDefinition> &f) {
    const std::unique_ptr<AST::FunctionPrototype> &proto = f->proto;
    // Check if the function is defined `extern`.
    llvm::Function *result = module->getFunction(proto->fname);

    if (!result) result = (*this)(proto);

    if (!result) return nullptr;

    if (!result->empty()) {
        return (llvm::Function *)log_error("Function cannot be redefined.");
    }

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", result);
    builder.SetInsertPoint(bb);

    names.clear();
    for (auto &arg: result->args()) {
        names[arg.getName()] = &arg;
    }

    if (llvm::Value *ret = boost::apply_visitor(expr_gen, f->body)) {
        builder.CreateRet(ret);
        llvm::verifyFunction(*result);

        return result;
    }

    result->eraseFromParent();
    return nullptr;
}

void CodeGenerator::emit_ir(std::ostream &out) {
    llvm::raw_os_ostream llvm_out(out);
    module->print(llvm_out, nullptr);
}

void CodeGenerator::emit_obj(int out) {
    llvm::raw_fd_ostream llvm_out(out, false);
    llvm::legacy::PassManager pass;
    auto ft = llvm::TargetMachine::CGFT_ObjectFile;

    if (target->addPassesToEmitFile(pass, llvm_out, ft)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
    }

    pass.run(*module);
    llvm_out.flush();
}

}
