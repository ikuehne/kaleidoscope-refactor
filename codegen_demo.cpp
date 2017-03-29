#include <iostream>

#include <boost/variant.hpp>

#include "AST.hh"
#include "Parser.hh"
#include "CodeGenerator.hh"

void handle_input(Kaleidoscope::Parser &p, Kaleidoscope::CodeGenerator &c) {
    auto e = p.parse();
    if (!is_err(e)) {
        boost::apply_visitor(c, e);
    }
}

int main(void) {
    Kaleidoscope::CodeGenerator codegen("my cool jit");
    Kaleidoscope::Parser parser;
    while (true) {
        if (parser.reached_end()) break;
        handle_input(parser, codegen);
    }

    codegen.emit_ir(std::cout);
}
