#pragma once

#include "../common/Fixed.h"

#include <string>

// Bounded exact rationals for static real expressions. Operations that exceed
// the checked 128-bit capacity fail explicitly instead of using host doubles.
struct ExactReal
{
    __int128 m_numerator = 0;
    __int128 m_denominator = 1;
    bool m_valid = false;

    static __int128 gcd(__int128 a, __int128 b)
    {
        if (a < 0) {
            a = -a;
        }
        if (b < 0) {
            b = -b;
        }
        while (b != 0) {
            __int128 next = a % b;
            a = b;
            b = next;
        }
        return a;
    }

    static ExactReal make(__int128 numerator, __int128 denominator = 1)
    {
        // Reserve the most-negative wide value so normalization is safe.
        const __int128 maximum = (((__int128)1 << 126) - 1) * 2 + 1;
        if (denominator == 0 || numerator < -maximum || denominator < -maximum) {
            return {};
        }
        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
        __int128 factor = gcd(numerator, denominator);
        return { numerator / factor, denominator / factor, true };
    }

    static ExactReal operation(char op, ExactReal left, ExactReal right)
    {
        if (!left.m_valid || !right.m_valid) {
            return {};
        }
        __int128 a = left.m_numerator, b = left.m_denominator;
        __int128 c = right.m_numerator, d = right.m_denominator;
        __int128 numerator, denominator, other;
        if (op == '+' || op == '-') {
            __int128 factor = gcd(b, d);
            if (__builtin_mul_overflow(a, d / factor, &numerator)
                || __builtin_mul_overflow(c, b / factor, &other)
                || (op == '+' ? __builtin_add_overflow(numerator, other, &numerator)
                              : __builtin_sub_overflow(numerator, other, &numerator))
                || __builtin_mul_overflow(b, d / factor, &denominator)) {
                return {};
            }
        } else {
            if (op == '/') {
                if (c == 0) {
                    return {};
                }
                __int128 swap = c;
                c = d;
                d = swap;
            }
            __int128 first = gcd(a, d), second = gcd(c, b);
            if (__builtin_mul_overflow(a / first, c / second, &numerator)
                || __builtin_mul_overflow(b / second, d / first, &denominator)) {
                return {};
            }
        }
        return make(numerator, denominator);
    }

    static ExactReal parse(std::string text)
    {
        std::string clean;
        for (char c : text) {
            if (c != '_') {
                clean += c;
            }
        }
        text = clean;
        int base = 10;
        std::size_t start = 0, end = text.size();
        std::size_t hash = text.find('#');
        std::size_t exponentAt;
        if (hash != std::string::npos) {
            base = std::stoi(text.substr(0, hash));
            start = hash + 1;
            end = text.find('#', start);
            exponentAt = end + 1;
        } else {
            exponentAt = text.find_first_of("eE");
            if (exponentAt != std::string::npos) {
                end = exponentAt;
            }
        }
        ExactReal value = make(0), radix = make(base), divisor = make(1);
        bool fraction = false;
        for (std::size_t i = start; i < end; ++i) {
            char c = text[i];
            if (c == '.') {
                fraction = true;
                continue;
            }
            int digit = c >= '0' && c <= '9' ? c - '0' : (c >= 'a' ? c - 'a' : c - 'A') + 10;
            value = operation('+', operation('*', value, radix), make(digit));
            if (fraction) {
                divisor = operation('*', divisor, radix);
            }
        }
        value = operation('/', value, divisor);
        if (exponentAt != std::string::npos && exponentAt < text.size()) {
            std::size_t pos = exponentAt + 1;
            bool negative = pos < text.size() && text[pos] == '-';
            if (pos < text.size() && (text[pos] == '-' || text[pos] == '+')) {
                ++pos;
            }
            int exponent = 0;
            for (; pos < text.size(); ++pos) {
                if (exponent > 128) {
                    return {};
                }
                exponent = exponent * 10 + text[pos] - '0';
            }
            if (exponent > 128) {
                return {};
            }
            while (exponent-- > 0) {
                value = operation(negative ? '/' : '*', value, radix);
            }
        }
        return value;
    }

    bool scaled(int bits, long long& result) const
    {
        ExactReal value = operation('*', *this, make((__int128)1 << bits));
        return value.m_valid && fixedRound(value.m_numerator, value.m_denominator, &result);
    }
};
