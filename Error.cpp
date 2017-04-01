#include "Error.hh"

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define TERM_ERR   "\x1b[31;1m"
#define TERM_IND   "\x1b[32;1m"
#define TERM_RESET "\x1b[0m"

namespace Kaleidoscope {

static std::map<std::string, std::unique_ptr<std::vector<std::string>>>
    files_read;

static std::vector<std::string> &get_lines(std::string f) {
    if (files_read.count(f)) {
        return *files_read[f];
    } else {
        auto v = std::make_unique<std::vector<std::string>>();
        std::ifstream file(f);
        char buf[81];
        while (!file.eof()) {
            unsigned i;
            char c;
            for (i = 0; i < 80; ++i) {
                if (file.eof()) break;
                c = file.get();
                if (c == '\n') {
                    break;
                }
                buf[i] = c;
            }
            if (c != '\n') {
                while (!file.eof() && file.get() != '\n');
            }
            buf[i] = '\0';
            v->push_back(std::string(buf));
        }
        files_read[f] = std::move(v);
        return *files_read[f];
    }
}

static void prev(uint16_t &lineno, uint16_t &charno,
                const std::vector<std::string> &lines ) {
    if (charno == 0) {
        if (lineno > 0) {
            lineno--;
            charno = lines[lineno].size() - 1;
        }
    }
}

Error::Error(std::string header, std::string msg, ErrorInfo info)
    : header(header), msg(msg), info(info) {}

void Error::emit(std::ostream &out) {
    assert(info.lineno_start <= info.lineno_end);
    const std::vector<std::string> &lines = get_lines(*info.filename);
    prev(info.lineno_end, info.charno_end, lines);

    assert(info.lineno_start <= lines.size());
    out << *info.filename
        << ":" << info.lineno_start << ":" << info.charno_start + 1
        << "-" << info.lineno_end << ":" << info.charno_end
        << ": " << TERM_ERR << header << ": " << TERM_RESET
        << msg << "\n\t"
        << lines[info.lineno_start] << "\n\t"
        << std::string(info.charno_start, ' ')
        << TERM_IND << "^" << TERM_RESET << std::endl;
    for (unsigned i = info.lineno_start + 1;
                  i <= info.lineno_end && i < lines.size(); ++i) {
        out << "\t" << lines[i] << std::endl;
    }
}

}
