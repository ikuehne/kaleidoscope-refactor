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

/* Actually the privileges most compilers create object files with. */
static const int OBJFILE_MODE_BLAZEIT = 420;

/**
 * @brief Pull a single AST out of the parser, and have the code generator
 *        visit it.
 */
void handle_input(Kaleidoscope::Parser &p, Kaleidoscope::CodeGenerator &c) {
    auto e = p.parse();
    boost::apply_visitor(c, e);
}

/**
 * @brief Entry point.
 */
int main(int argc, char **argv) {
    /* Boost command-line option stuff... */
    opt::options_description desc("Kaleidoscope Compiler Options");
    desc.add_options()
        ("help", "print usage information")
        ("obj", opt::value<std::string>(),
            "select output file to emit object code")
        ("ll", opt::value<std::string>(),
            "select output file to emit LLVM IR")
        ("in", opt::value<std::string>(), "select input file");
    opt::positional_options_description pos;
    pos.add("in", -1);

    opt::variables_map opt_map;
    try {
        opt::store(opt::command_line_parser(argc, argv).options(desc)
                                                       .positional(pos)
                                                       .run(),
                   opt_map);
    } catch (opt::error) {
        std::cerr << desc << std::endl;
        return 1;
    }
    opt::notify(opt_map);

    /* If the user did good, */
    if (!opt_map.count("help")
      && (opt_map.count("obj") || opt_map.count("ll"))
      && opt_map.count("in")) {
        /* Get a code generator. */
        Kaleidoscope::CodeGenerator codegen("Kaleidoscope module");
        /* Open the source file. */
        std::string infile(opt_map["in"].as<std::string>());
        /* Construct a parser on that file. */
        Kaleidoscope::Parser parser(infile);
        /* Pull ASTs out of the parser */
        while (true) {
            /* until we hit EOF. */
            if (parser.reached_end()) break;
            handle_input(parser, codegen);
        }

        if (opt_map.count("obj")) {
            /* Open the output file (LLVM's stream formats are weird, so we
             * can't use regular STL stream classes). */
            int fd = open(opt_map["obj"].as<std::string>().c_str(),
                          O_RDWR | O_CREAT,
                          OBJFILE_MODE_BLAZEIT);
            /* Emit the object code. */
            codegen.emit_obj(fd);
            close(fd);
        }
        if (opt_map.count("ll")) {
            std::ofstream file(opt_map["ll"].as<std::string>());
            codegen.emit_ir(file);
        }
    } else {
        /* Print usage information if the user did bad. */
        std::cerr << desc << std::endl;
        return 1;
    }

}
