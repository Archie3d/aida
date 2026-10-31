#pragma once

#include "../common/Fixed.h"

#include <string>
#include <climits>
#include <cmath>

// Bounded exact rationals for static real expressions. Operations that exceed
// the checked 128-bit capacity fail explicitly instead of using host doubles.
struct ExactReal
{
    __int128 numerator = 0;
    __int128 denominator = 1;
    bool valid = false;

    bool integer(long long& result) const
    {
        if (!valid || denominator != 1 || numerator < LLONG_MIN || numerator > LLONG_MAX) {
            return false;
        }
        result = static_cast<long long>(numerator);
        return true;
    }

    // Round the exact fraction once to a binary machine value, nearest/even.
    // Generate significand bits by division so neither cross-products nor a
    // preliminary host-double rounding can lose a bit at a Float midpoint.
    double real(int precision = 53) const
    {
        if (!valid || numerator == 0) {
            return 0.0;
        }
        using Magnitude = unsigned __int128;
        Magnitude numeratorMagnitude = numerator < 0 ? -numerator : numerator;
        Magnitude denominatorMagnitude = denominator;
        auto bits = [](Magnitude value) {
            int count = 0;
            for (; value != 0; value >>= 1) {
                ++count;
            }
            return count;
        };
        int exponent = bits(numeratorMagnitude) - bits(denominatorMagnitude);
        if (exponent >= 0 ? numeratorMagnitude < (denominatorMagnitude << exponent)
                          : (numeratorMagnitude << -exponent) < denominatorMagnitude) {
            --exponent;
        }
        if (exponent >= 0) {
            denominatorMagnitude <<= exponent;
        } else {
            numeratorMagnitude <<= -exponent;
        }
        // A supported rational can reach Float subnormals, but not Double
        // subnormals. Reduce precision to preserve the Float subnormal grid.
        int digits = precision == 24 && exponent < -126 ? precision + exponent + 126 : precision;
        unsigned long long significand = 1;
        Magnitude remainder = numeratorMagnitude - denominatorMagnitude;
        for (int i = 1; i < digits; ++i) {
            remainder <<= 1;
            significand <<= 1;
            if (remainder >= denominatorMagnitude) {
                remainder -= denominatorMagnitude;
                significand |= 1;
            }
        }
        if (remainder > denominatorMagnitude - remainder
            || (remainder == denominatorMagnitude - remainder && (significand & 1))) {
            ++significand;
        }
        double result = std::ldexp(static_cast<double>(significand), exponent - digits + 1);
        return numerator < 0 ? -result : result;
    }

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
        const __int128 kMaximum = (((__int128)1 << 126) - 1) * 2 + 1;
        if (denominator == 0 || numerator < -kMaximum || denominator < -kMaximum) {
            return {};
        }
        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
        __int128 factor = gcd(numerator, denominator);
        return { numerator / factor, denominator / factor, true };
    }

    // Recover a machine value exactly when it fits the rational capacity.
    static ExactReal binary(double value)
    {
        if (!std::isfinite(value)) { return {}; }
        if (value == 0.0) { return make(0); }
        int exponent = 0;
        double fraction = std::frexp(value, &exponent);
        __int128 numerator = static_cast<long long>(std::ldexp(fraction, 53));
        exponent -= 53;
        while (exponent < 0 && numerator % 2 == 0) {
            numerator /= 2;
            ++exponent;
        }
        if (exponent < -126 || exponent > 126) { return {}; }
        if (exponent < 0) { return make(numerator, (__int128)1 << -exponent); }
        __int128 scaled;
        return __builtin_mul_overflow(numerator, (__int128)1 << exponent, &scaled) ? ExactReal{} : make(scaled);
    }

    static ExactReal operation(char op, ExactReal left, ExactReal right)
    {
        if (!left.valid || !right.valid) {
            return {};
        }
        __int128 a = left.numerator, b = left.denominator;
        __int128 c = right.numerator, d = right.denominator;
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
            // The lexer diagnoses malformed bases; never throw while recovering.
            base = 0;
            for (std::size_t i = 0; i < hash; ++i) {
                if (text[i] < '0' || text[i] > '9' || base > 16) {
                    return {};
                }
                base = base * 10 + text[i] - '0';
            }
            if (base < 2 || base > 16) {
                return {};
            }
            start = hash + 1;
            end = text.find('#', start);
            if (end == std::string::npos) {
                return {};
            }
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
        if (!value.valid || value.numerator == 0) {
            return value;
        }
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

    static ExactReal fixed(long long counts, int scale)
    {
        int power = fixedTwos(scale);
        __int128 factor = fixedIsDecimal(scale) ? fixedPower10(power < 0 ? -power : power)
            : (__int128)1 << (power < 0 ? -power : power);
        return power >= 0 ? make(counts, factor) : make((__int128)counts * factor);
    }

    bool scaled(int bits, long long& result) const
    {
        return valid && fixedValidScale(bits)
            && fixedScaleRatio(numerator, denominator, fixedTwos(bits), fixedFives(bits), 0, 0, &result);
    }
};
