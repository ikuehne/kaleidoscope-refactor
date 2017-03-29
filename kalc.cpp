#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <boost/program_options.hpp>
#include <boost/variant.hpp>

#include "AST.hh"
#include "Parser.hh"
#include "CodeGenerator.hh"

namespace opt = boost::program_options;

static const int OBJFILE_MODE_BLAZEIT = 420;

void handle_input(Kaleidoscope::Parser &p, Kaleidoscope::CodeGenerator &c) {
    auto e = p.parse();
    if (!Kaleidoscope::AST::is_err(e)) {
        boost::apply_visitor(c, e);
    }
}

int main(int argc, char **argv) {
    opt::options_description desc("Kaleidoscope Compiler Options");
    desc.add_options()
        ("help", "print usage information")
        ("out", opt::value<std::string>(),
            "select output file to emit object code")
        ("in", opt::value<std::string>(), "select input file");
    opt::positional_options_description pos;
    pos.add("in", -1);

    opt::variables_map opt_map;
    opt::store(opt::command_line_parser(argc, argv).options(desc)
                                                   .positional(pos)
                                                   .run(),
               opt_map);
    opt::notify(opt_map);

    if (!opt_map.count("help")
      && opt_map.count("out")
      && opt_map.count("in")) {
        Kaleidoscope::CodeGenerator codegen("Kaleidoscope module");
        std::ifstream infile(opt_map["in"].as<std::string>());
        Kaleidoscope::Parser parser(&infile);
        while (true) {
            if (parser.reached_end()) break;
            handle_input(parser, codegen);
        }

        int fd = open(opt_map["out"].as<std::string>().c_str(),
                      O_RDWR | O_CREAT,
                      OBJFILE_MODE_BLAZEIT);
        codegen.emit_obj(fd);
        close(fd);
    } else {
        std::cerr << desc << std::endl;
        return 1;
    }

}
