#pragma once

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>

namespace Kaleidoscope {

/* It's very important to keep these small, as they are used by every leaf in
 * the AST.  Currently 128 bits.  (clang somehow gets its equivalent structure
 * down to 32 bits...) */
struct ErrorInfo {
    std::shared_ptr<std::string> filename;
    uint16_t lineno_start;
    uint16_t charno_start;
    uint16_t lineno_end;
    uint16_t charno_end;

    ErrorInfo(std::shared_ptr<std::string> f, uint16_t cs, uint16_t ls,
                                              uint16_t ce, uint16_t le)
        : filename(f), lineno_start(ls), charno_start(cs),
                       lineno_end(le), charno_end(ce) {}
};

template <typename T> using Annotated = std::pair<ErrorInfo, T>;

class Error {
public:
    Error(std::string, std::string, ErrorInfo);
    void emit(std::ostream &);

private:
    std::string header;
    std::string msg;
    ErrorInfo info;
};

}
