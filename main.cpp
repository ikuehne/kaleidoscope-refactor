#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include <boost/variant.hpp>

#include "AST.hh"
#include "Parser.hh"
#include "CodeGenerator.hh"

void handle_input(Kaleidoscope::Parser &p, Kaleidoscope::CodeGenerator &c) {
    auto e = p.parse();
    if (!Kaleidoscope::AST::is_err(e)) {
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

    int fd = open("a.out", O_RDWR | O_CREAT);
    codegen.emit_obj(fd);
    close(fd);
}
