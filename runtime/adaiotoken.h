#ifndef ADA_IO_TOKEN_H
#define ADA_IO_TOKEN_H

/* Lexical candidates; the type's value parser validates digits, separators,
   matching radix delimiters and range. Kind: integer, real, enumeration. */
static inline int ioTokenState(int state, int c, int kind)
{
    int digit = c >= '0' && c <= '9';
    int hex = digit || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    int letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 192;
    if (kind == 2) {
        if (state == 0 && (c == ' ' || c == '\t')) { return 0; }
        if (state == 0 && c == '\'') { return 10; }
        if (state == 10) { return 11; }
        if (state == 11) { return c == '\'' ? 12 : -1; }
        if (state == 12) { return -1; }
        return letter || (state == 20 && (digit || c == '_')) ? 20 : -1;
    }
    switch (state) {
    case 0:
        if (c == ' ' || c == '\t') { return 0; }
        if (c == '+' || c == '-') { return 1; }
        /* fall through */
    case 1:
        if (digit) { return 2; }
        return kind == 1 && c == '.' ? 8 : -1;
    case 2:
        if (digit || c == '_') { return 2; }
        if (c == '#' || c == ':') { return 3; }
        if (kind == 1 && c == '.') { return 8; }
        return c == 'e' || c == 'E' ? 5 : -1;
    case 3:
    case 9:
        if (hex || c == '_') { return state; }
        if (kind == 1 && state == 3 && c == '.') { return 9; }
        return c == '#' || c == ':' ? 4 : -1;
    case 4: return c == 'e' || c == 'E' ? 5 : -1;
    case 8:
        if (digit || c == '_') { return 8; }
        return c == 'e' || c == 'E' ? 5 : -1;
    case 5:
        if (c == '+' || c == '-') { return 6; }
        /* fall through */
    case 6:
    case 7: return digit || c == '_' ? 7 : -1;
    default: return -1;
    }
}

#endif
