#include <iostream>

#include "AST.hh"
#include "Parser.hh"
#include "CodeGenerator.hh"

void handle_input(Kaleidoscope::Parser &p, Kaleidoscope::CodeGenerator &c) {
    if (auto e = p.parse()) {
        e->host_generator(c);
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
