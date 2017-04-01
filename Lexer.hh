#pragma once

#include <fstream>
#include <istream>
#include <iostream>
#include <memory>
#include <string>

#include "Error.hh"

namespace Kaleidoscope {

/**
 * @brief Identifies known tokens.  See `Lexer::get_token`.
 */
enum Token {
    /** End of input has been reached. */
    tok_eof = -1,

    /** Function definition. */
    tok_def = -2,

    /** Extern declaration. */
    tok_extern = -3,

    /** Identifier (variable or function name). */
    tok_identifier = -4,

    /** Floating-point literal. */
    tok_number = -5,

    /** If condition. */
    tok_if = -6,

    /** Then expression. */
    tok_then = -7,

    /** Else expression. */
    tok_else = -8,

    /** For loop. */
    tok_for = -9,

    /** "in" part of for loop. */
    tok_in = -10,
};

/**
 * @brief Lexer over an input stream.
 */
class Lexer {
public:

    /**
     * @brief Create a new lexer based on the given input stream.
     *
     * @param input Input stream to lex.
     */
    inline Lexer(std::string f)
        : identifier(), number(0.0), lineno(0),
          charno(0), old_lineno(0), old_charno(0) {
        fname = std::make_shared<std::string>(f);
        input = std::make_unique<std::ifstream>(f);
    }

    /**
     * @brief Lex the next token from the input stream.
     *
     * Also sets the instance's identifier/number value if appropriate; see
     * `get_identifier` and `get_number`.
     *
     * @return A character 0-255 if an unknown character, otherwise a member
     *         of Kaleidoscope::Token.
     */
    Annotated<int> get_token(void);

    /**
     * @brief Return the last identifier lexed with `get_token`.
     */
    inline std::string get_identifier(void) const { return identifier; }

    /**
     * @brief Return the last number lexed with `get_token`.
     */
    inline double get_number(void) const { return number; }

private:
    int get_char(void);

    std::string identifier;
    double number;
    std::shared_ptr<std::string> fname;
    std::unique_ptr<std::istream> input;
    int old_lineno;
    int old_charno;
    int lineno;
    int charno;
};

}
