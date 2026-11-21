#include <stddef.h>
#include <stdint.h>

typedef void (*Fill)(char*, wchar_t*, uint_least16_t*, uint_least32_t*, const char*,
                     size_t, int, short, unsigned char, double, unsigned short*);

void aida_c_fill(char* text, wchar_t* wide, uint_least16_t* units16, uint_least32_t* units32,
                 const char* source, size_t count, int guard, short tail, unsigned char last,
                 double scale, unsigned short* status)
{
    if (count != 3 || guard != 17009 || tail != -301 || last != 237 || scale != 2.5
        || source[0] != 'A' || source[1] != 0 || source[2] != 'Z'
        || text[0] != 'x' || wide[0] != 0x1234) {
        *status = 0;
        return;
    }
    text[0] = source[2];
    text[1] = (char)0xE9;
    text[2] = 0;
    wide[1] = 0xFEDC;
    units16[0] = 0xD800;
    units16[1] = 0xF123;
    units32[0] = 0x1F642;
    units32[1] = 0x10FFFF;
    *status = 0xE205;
}

Fill aida_c_get_fill(void)
{
    return aida_c_fill;
}

unsigned aida_c_words(unsigned* words, size_t count)
{
    if (count == 0) {
        return 55;
    }
    if (count != 3 || words[1] != 0x80000000U) {
        return 0;
    }
    words[2] = 0xF1234567U;
    return words[0];
}

int* aida_c_pointer(int* value, int expected)
{
    return value != NULL && *value == expected ? value : NULL;
}

int aida_c_read_char(const char* value)
{
    return value == NULL ? -1 : (unsigned char)*value;
}

void aida_c_replace_pointer(int** value)
{
    static int replacement = 321;
    *value = &replacement;
}

void aida_c_update(int* value, int replacement)
{
    *value = replacement;
}

void aida_c_set_enum(int* value)
{
    *value = *value == -20 ? 241 : 0;
}

void aida_c_returns(char* text, unsigned char value)
{
    text[0] = (char)value;
}
