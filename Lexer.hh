#pragma once

#include <istream>
#include <iostream>
#include <string>

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
    inline Lexer(std::istream *input = &std::cin) {
        this->input = input;
        identifier = std::string();
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
    int get_token(void);

    /**
     * @brief Return the last identifier lexed with `get_token`.
     */
    inline std::string get_identifier(void) const { return identifier; };

    /**
     * @brief Return the last number lexed with `get_token`.
     */
    inline double get_number(void) const { return number; };

private:
    std::string identifier;
    double number;
    std::istream *input;
};

}
