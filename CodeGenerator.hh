#pragma once

#include <memory>
#include <string>
#include <iostream>

#include <boost/variant.hpp>
#include "llvm/Support/Host.h"

#include "AST.hh"

namespace Kaleidoscope {

class CodeGeneratorImpl;
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

    ~CodeGenerator();

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

    std::unique_ptr<CodeGeneratorImpl> pimpl;
};

}
