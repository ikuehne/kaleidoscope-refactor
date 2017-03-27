#include <iostream>
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_os_ostream.h"

#include "CodeGenerator.hh"

namespace Kaleidoscope {

static llvm::Value *log_error(const char *str) {
    std::cerr << "Kaleidoscope::CodeGenerator::log_error: "
              << str << std::endl;
    return nullptr;
}

CodeGenerator::CodeGenerator(std::string name)
    : builder(context),
      module(llvm::make_unique<llvm::Module>(name, context)) {}

llvm::Value *CodeGenerator::codegen_value(const AST::NumberLiteral &num) {
    /* Create a floating-point constant with this value in this context. */
    return llvm::ConstantFP::get(context, llvm::APFloat(num.get()));
}

llvm::Value *CodeGenerator::codegen_value(const AST::VariableName &var) {
    /* Just look up the value corresponding to this name, and return that. */
    auto result = names[var.get()];
    if (!result) return log_error("Unknown variable name");
    return result;
}

llvm::Value *CodeGenerator::codegen_value(const AST::BinaryOp &op) {
    /* Get the LLVM values for left and right. */
    llvm::Value *l = op.get_lhs().host_generator(*this);
    llvm::Value *r = op.get_rhs().host_generator(*this);
    if (!l || !r) return nullptr;

    switch(op.get_op()) {
    case '+':
        return builder.CreateFAdd(l, r, "addtmp");
    case '-':
        return builder.CreateFSub(l, r, "subtmp");
    case '*':
        return builder.CreateFMul(l, r, "multmp");
    case '<':
        l = builder.CreateFCmpULT(l, r, "cmptmp");
        /* Convert bool 0/1 to double 0.0 or 1.0 */
        return builder.CreateUIToFP(
                l, llvm::Type::getDoubleTy(context), "booltmp");
    default:
        return log_error("invalid binary operator.");
    }
}

llvm::Value *CodeGenerator::codegen_value(const AST::FunctionCall &call) {
	/* Look up the name in the global module table. */
    llvm::Function *llvm_func = module->getFunction(call.get_name());
	if (!llvm_func)
		return log_error("Unknown function referenced");

    /* Log argument mismatch error. */
    if (llvm_func->arg_size() != call.get_args().size())
        return log_error("Incorrect # arguments passed");

    std::vector<llvm::Value *> llvm_args;
    for (unsigned i = 0; i != call.get_args().size(); ++i) {
        llvm_args.push_back(call.get_args()[i]->host_generator(*this));
        if (!llvm_args.back()) return nullptr;
    }

    return builder.CreateCall(llvm_func, llvm_args, "calltmp");
}

llvm::Function *
      CodeGenerator::codegen_func(const AST::FunctionPrototype &func) {
    std::vector<llvm::Type *> doubles(func.get_args().size(),
                                      llvm::Type::getDoubleTy(context));
    llvm::FunctionType *ft =
        llvm::FunctionType::get(llvm::Type::getDoubleTy(context),
                                doubles, false);

    llvm::Function *result =
            llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                   func.get_name(), module.get());
    unsigned i = 0;
    for (auto &arg: result->args())
        arg.setName(func.get_args()[i++]);

    return result;
}

llvm::Function *
      CodeGenerator::codegen_func(const AST::FunctionDefinition &f) {
    const AST::FunctionPrototype &proto = f.get_prototype();
    // Check if the function is defined `extern`.
    llvm::Function *result = module->getFunction(proto.get_name());

    if (!result) result = proto.host_generator(*this);

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

    if (llvm::Value *ret = f.get_body().host_generator(*this)) {
        builder.CreateRet(ret);
        llvm::verifyFunction(*result);

        return result;
    }

    result->eraseFromParent();
    return nullptr;
}

void CodeGenerator::emit(std::ostream &out) {
    llvm::raw_os_ostream llvm_out(out);
    module->print(llvm_out, nullptr);
}

}
