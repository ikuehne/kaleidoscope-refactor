#include <cstdlib>
#include <cctype>
#include "Lexer.hh"

namespace Kaleidoscope {

int Lexer::get_token(void) {
    static int last_char = ' ';

    // Skip any whitespace.
    while (isspace(last_char)) {
        last_char = input->get();
    }

    if (isalpha(last_char)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
        identifier = last_char;
        for (last_char = input->get();
             isalnum(last_char);
             last_char = input->get()) {
            identifier += last_char;
        }

        if (identifier == "def")
            return tok_def;
        if (identifier == "extern")
            return tok_extern;
        return tok_identifier;
    }

    if (isdigit(last_char) || last_char == '.') {   // Number: [0-9.]+
        std::string number_string;
        do {
            number_string += last_char;
            last_char = input->get();
        } while (isdigit(last_char) || last_char == '.');
        number = strtod(number_string.c_str(), 0);
        return tok_number;
    }

    // Skip comments until the end of the line.
    if (last_char == '#') {
        do {
            last_char = input->get();
        } while (last_char != EOF && last_char != '\n' && last_char != '\r');

        if (last_char != EOF) {
            return get_token();
        }
    }

    if (last_char == EOF) return tok_eof;

    int this_char = last_char;
    last_char = input->get();
    return this_char;
}

}
