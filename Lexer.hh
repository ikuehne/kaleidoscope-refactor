#pragma once

#include <istream>
#include <iostream>
#include <string>

namespace Kaleidoscope {

enum Token {
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,
};

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
