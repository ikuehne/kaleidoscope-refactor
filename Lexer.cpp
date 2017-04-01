#include <cstdlib>
#include <cctype>
#include "Lexer.hh"

namespace Kaleidoscope {

Annotated<int> Lexer::get_token(void) {
    static int last_char = ' ';

    // Skip any whitespace.
    while (isspace(last_char)) {
        last_char = get_char();
    }

    ErrorInfo info(fname, old_charno, old_lineno, old_charno, old_lineno);

    /* An identifier starts with an alphanumeric character, */
    if (isalpha(last_char)) {
        identifier = last_char;
        /* and continues with alphanumeric characters. */
        for (last_char = get_char();
             isalnum(last_char);
             last_char = get_char()) {
            /* Collect the name of the identifier. */
            identifier += last_char;
        }

        info.lineno_end = old_lineno;
        info.charno_end = old_charno;

        /* Could be a definition, */
        if (identifier == "def")    return Annotated<int>(info, tok_def);
        /* an extern declaration, */
        if (identifier == "extern") return Annotated<int>(info, tok_extern);
        /* one of the bits of an "if" block, */
        if (identifier == "if")     return Annotated<int>(info, tok_if);
        if (identifier == "then")   return Annotated<int>(info, tok_then);
        if (identifier == "else")   return Annotated<int>(info, tok_else);
        /* one of the bits of a "for" loop, */
        if (identifier == "for")    return Annotated<int>(info, tok_for);
        if (identifier == "in")     return Annotated<int>(info, tok_in);
        /* or an identifier. */
        return Annotated<int>(info, tok_identifier);
    }

    /* Numbers consist of digits and decimals. */
    if (isdigit(last_char) || last_char == '.') {
        std::string number_string;
        do {
            number_string += last_char;
            last_char = get_char();
        } while (isdigit(last_char) || last_char == '.');

        info.lineno_end = old_lineno;
        info.charno_end = old_charno;

        /* Store the lexed number as a float. */
        number = strtod(number_string.c_str(), 0);
        return Annotated<int>(info, tok_number);
    }

    /* Skip comments until the end of the line. */
    if (last_char == '#') {
        do {
            last_char = get_char();
        } while (last_char != EOF && last_char != '\n' && last_char != '\r');

        if (last_char != EOF) {
            return get_token();
        }
    }

    if (last_char == EOF) return Annotated<int>(info, tok_eof);

    /* If unknown, just return the character. */
    int this_char = last_char;
    last_char = get_char();
    return Annotated<int>(info, this_char);
}

int Lexer::get_char(void) {
    int next = input->get();
    old_lineno = lineno;
    old_charno = charno;

    if (next == '\n' || next == '\r') {
        ++lineno;
        charno = 0;
    } else {
        ++charno;
    }

    return next;
}

}
