#include "CodeGenerator.hh"
#include "CodeGeneratorImpl.hh"

namespace Kaleidoscope {

/*
 * Thin wrapper over CodeGeneratorImpl.  See CodeGeneratorImpl.cpp for real
 * implementations of these methods.
 */

CodeGenerator::CodeGenerator(std::string name,
                             std::string triple)
    : pimpl(std::make_unique<CodeGeneratorImpl>(name, triple)) {}

CodeGenerator::~CodeGenerator() = default;

llvm::Function *CodeGenerator::operator()(const AST::Declaration &decl) {
    return boost::apply_visitor(*pimpl, decl);
}

void CodeGenerator::emit_ir(std::ostream &out) {
    return pimpl->emit_ir(out);
}

void CodeGenerator::emit_obj(int out) {
    return pimpl->emit_obj(out);
}

}
