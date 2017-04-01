#include <iostream>
#include <memory>
#include <sstream>

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

#include "CodeGeneratorImpl.hh"

namespace Kaleidoscope {

/*****************************************************************************
 * Utilities.
 */

[[noreturn]] static void _throw(std::string msg, ErrorInfo info) {
    throw Error("Codegen error", msg, info);
}

static llvm::Value *log_error(std::string str) {
    std::cerr << "Kaleidoscope::CodeGenerator::log_error: "
              << str << std::endl;
    return nullptr;
}

/*****************************************************************************
 * ExpressionGenerator implementation.
 */

llvm::Value *ExpressionGenerator::to_cond(llvm::Value *f) {
    if (!f) return nullptr;
    return builder.CreateFCmpONE(f,
                                 llvm::ConstantFP::get(context,
                                                       llvm::APFloat(0.0)),
                                 "cond");
}

llvm::Value *ExpressionGenerator::operator()(const AST::NumberLiteral &num) {
    /* Create a floating-point constant with this value in this context. */
    return llvm::ConstantFP::get(context, llvm::APFloat(num.val));
}

llvm::Value *ExpressionGenerator::operator()(const AST::VariableName &var) {
    /* Just look up the value corresponding to this name, and return that. */
    auto result = names[var.name];
    if (!result) {
        _throw("unknown variable name (" + var.name + ")", var.info);
    }
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
        _throw(std::string("invalid binary operator (")
             + op->op + ")", op->info);
    }
}

llvm::Value *ExpressionGenerator::operator()(
        const std::unique_ptr<AST::FunctionCall> &call) {
	/* Look up the name in the global module table. */
    llvm::Function *llvm_func = module.getFunction(call->fname);
	if (!llvm_func) {
        _throw("unknown function referenced: " + call->fname, call->info);
    }

    /* Log argument mismatch error. */
    if (llvm_func->arg_size() != call->args.size()) {
        _throw("incorrect # of arguments passed", call->info);
    }

    std::vector<llvm::Value *> llvm_args;
    for (unsigned i = 0; i != call->args.size(); ++i) {
        llvm_args.push_back(boost::apply_visitor(*this, call->args[i]));
        if (!llvm_args.back()) return nullptr;
    }

    return builder.CreateCall(llvm_func, llvm_args, "calltmp");
}

llvm::Value *ExpressionGenerator::operator()(
        const std::unique_ptr<AST::IfThenElse> &if_) {

    /* Generate code for the condition. */
    llvm::Value *cond = to_cond(boost::apply_visitor(*this, if_->cond));
    if (!cond) return nullptr;

    /* Get the parent function (so that the builder knows where to do stuff).
    */
    llvm::Function *parent = builder.GetInsertBlock()->getParent();

    /* Create a then block (with nothing in it), so that we can reference it
     * in the branch.  Note that this actually emits the block. */
    auto *then_bb = llvm::BasicBlock::Create(context, "then", parent);
    /* Ditto with else. */
    auto *else_bb = llvm::BasicBlock::Create(context, "else");
    /* Block jumped to after `then_bb` or `else_bb`. */
    auto *merge_bb = llvm::BasicBlock::Create(context, "merge");
    /* Create a conditional branch that jumps to one of the above blocks. */
    builder.CreateCondBr(cond, then_bb, else_bb);

    /* Generate code for the "then" block. */
    builder.SetInsertPoint(then_bb);
    llvm::Value *then = boost::apply_visitor(*this, if_->then);
    if (!then) return nullptr;

    /* After "then" is done, jump (past "else") to "merge". */
    builder.CreateBr(merge_bb);
    then_bb = builder.GetInsertBlock();

    /* Emit the "else" block. */
    parent->getBasicBlockList().push_back(else_bb);

    /* Generate code for the "then" block. */
    builder.SetInsertPoint(else_bb);
    llvm::Value *else_ = boost::apply_visitor(*this, if_->else_);
    if (!else_) return nullptr;

    builder.CreateBr(merge_bb);
    else_bb = builder.GetInsertBlock();

    /* Emit the "merge" block. */
    parent->getBasicBlockList().push_back(merge_bb);

    /* Generate code for the "merge" block. */
    builder.SetInsertPoint(merge_bb);
    /* This block just returns the result of a phi node. */
    llvm::PHINode *pn = builder.CreatePHI(llvm::Type::getDoubleTy(context),
                                          2, "iftemp");
    pn->addIncoming(then, then_bb);
    pn->addIncoming(else_, else_bb);

    return pn;
}

llvm::Value *ExpressionGenerator::operator()(
        const std::unique_ptr<AST::ForLoop> &loop) {
    llvm::Function *parent = builder.GetInsertBlock()->getParent();
    auto &entry_bb = parent->back();
    auto *loop_bb = llvm::BasicBlock::Create(context, "loop", parent);
    auto *exit_bb = llvm::BasicBlock::Create(context, "loop_exit");
    auto start = boost::apply_visitor(*this, loop->start);

    builder.CreateBr(loop_bb);

    builder.SetInsertPoint(loop_bb);
    auto loop_idx = builder.CreatePHI(llvm::Type::getDoubleTy(context),
                                      2, loop->index_var);
    loop_idx->addIncoming(start, &entry_bb);
    auto *old_val = names[loop->index_var];
    /* Delay adding the other incoming until we finish "loop". */
    names[loop->index_var] = loop_idx;
    /* Discard value body evaluates to. */
    if (!boost::apply_visitor(*this, loop->body)) return nullptr;
    auto step = boost::apply_visitor(*this, loop->step);
    auto next = builder.CreateFAdd(loop_idx, step);

    auto end = to_cond(boost::apply_visitor(*this, loop->end));
    if (!end) return nullptr;

    auto *last_loop_bb = builder.GetInsertBlock();

    loop_idx->addIncoming(next, last_loop_bb);

    builder.CreateCondBr(end, loop_bb, exit_bb);

    builder.SetInsertPoint(exit_bb);

    parent->getBasicBlockList().push_back(exit_bb);

    /* Delete the index variable from the environment. */
    if (old_val) {
        names[loop->index_var] = old_val;
    } else {
        names.erase(loop->index_var);
    }
    
    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(context));
}

/*****************************************************************************
 * CodeGeneratorImpl implementations.
 */

CodeGeneratorImpl::CodeGeneratorImpl(std::string name, std::string triple)
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

llvm::Function *CodeGeneratorImpl::operator()
       (const std::unique_ptr<AST::FunctionPrototype> &func) {
    assert(func);
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

llvm::Function *CodeGeneratorImpl::operator()
            (const std::unique_ptr<AST::FunctionDefinition> &f) {
    const AST::FunctionPrototype &proto = *f->proto;
    assert(module != nullptr);
    // Check if the function is defined `extern`.
    llvm::Function *result = module->getFunction(proto.fname);

    if (!result) result = (*this)(f->proto);

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

void CodeGeneratorImpl::emit_ir(std::ostream &out) {
    llvm::raw_os_ostream llvm_out(out);
    module->print(llvm_out, nullptr);
}

void CodeGeneratorImpl::emit_obj(int out) {
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
