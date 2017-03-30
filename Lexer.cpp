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

    /* An identifier starts with an alphanumeric character, */
    if (isalpha(last_char)) {
        identifier = last_char;
        /* and continues with alphanumeric characters. */
        for (last_char = input->get();
             isalnum(last_char);
             last_char = input->get()) {
            /* Collect the name of the identifier. */
            identifier += last_char;
        }

        /* Could be a definition, */
        if (identifier == "def")    return tok_def;
        /* an extern declaration, */
        if (identifier == "extern") return tok_extern;
        /* one of the bits of an "if" block, */
        if (identifier == "if")     return tok_if;
        if (identifier == "then")   return tok_then;
        if (identifier == "else")   return tok_else;
        /* or an identifier. */
        return tok_identifier;
    }

    /* Numbers consist of digits and decimals. */
    if (isdigit(last_char) || last_char == '.') {
        std::string number_string;
        do {
            number_string += last_char;
            last_char = input->get();
        } while (isdigit(last_char) || last_char == '.');
        /* Store the lexed number as a float. */
        number = strtod(number_string.c_str(), 0);
        return tok_number;
    }

    /* Skip comments until the end of the line. */
    if (last_char == '#') {
        do {
            last_char = input->get();
        } while (last_char != EOF && last_char != '\n' && last_char != '\r');

        if (last_char != EOF) {
            return get_token();
        }
    }

    if (last_char == EOF) return tok_eof;

    /* If unknown, just return the character. */
    int this_char = last_char;
    last_char = input->get();
    return this_char;
}

}
