#ifndef ADA_MODULAR_TEXT_H
#define ADA_MODULAR_TEXT_H

/* Candidate integer-literal scanner. The value parser subsequently validates
   separators, radix digits, exponent and range. A negative result preserves
   the first character after the token for the next input operation. */
static inline int modularTokenState(int state, int character)
{
    int decimal = character >= '0' && character <= '9';
    int hexadecimal = decimal || (character >= 'a' && character <= 'f')
        || (character >= 'A' && character <= 'F');
    switch (state) {
    case 0:
        if (character == ' ' || character == '\t') { return 0; }
        if (character == '+' || character == '-') { return 1; }
        return decimal ? 2 : -1;
    case 1: return decimal ? 2 : -1;
    case 2:
        if (decimal || character == '_') { return 2; }
        if (character == '#') { return 3; }
        return character == 'e' || character == 'E' ? 5 : -1;
    case 3:
        if (hexadecimal || character == '_') { return 3; }
        return character == '#' ? 4 : -1;
    case 4: return character == 'e' || character == 'E' ? 5 : -1;
    case 5:
        if (character == '+' || character == '-') { return 6; }
        return decimal || character == '_' ? 7 : -1;
    case 6:
    case 7: return decimal || character == '_' ? 7 : -1;
    default: return -1;
    }
}

#endif
