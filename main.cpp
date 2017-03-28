#include <fcntl.h>
#include <iostream>
#include <unistd.h>

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

    int fd = open("a.out", O_RDWR | O_CREAT);
    codegen.emit_obj(fd);
    close(fd);
}
