/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

// TODO : check if the models have different exponent offsets

#include "TypeHandlers.h"
#include "../tivarslib_utils.h"
#include <regex>

static bool parseSign(std::string::const_iterator &i, const std::string::const_iterator &e) {
    bool sign = false;
    if (i != e && (*i == '+' || *i == '-')) {
        sign = *i++ == '-';
    }
    if (i == e) {
        throw std::invalid_argument("Unexpected end of string.");
    }
    return sign;
}

namespace tivars
{
    data_t STH_FP::makeDataFromString(const std::string& str, const options_t& options)
    {
        (void)options;

        data_t data(dataByteCount);
        bool beforePoint = true, noDigits = true, zero = true;
        int exponent = 0x7F;
        unsigned index = 4;
        std::string::const_iterator i = str.begin();
        const std::string::const_iterator e = str.end();
        if (parseSign(i, e)) {
            data[0] = 1 << 7;
        }
        do {
            char c = *i++;
            if (c == '.') {
                if (!beforePoint) {
                    throw std::invalid_argument("Extra decimal points.");
                }
                beforePoint = false;
            } else if (c == '0') {
                noDigits = false;
                if (zero) {
                    exponent -= !beforePoint;
                } else {
                    exponent += beforePoint;
                    index += index <= dataByteCount << 1;
                }
            } else if (c >= '1' && c <= '9') {
                noDigits = zero = false;
                exponent += beforePoint;
                if (index < dataByteCount << 1) {
                    data[index >> 1] |= (c - '0') << ((~index & 1) << 2);
                    index++;
                } else if (index == dataByteCount << 1) {
                    if (c >= '5') {
                        while (true) {
                            if (--index < 4) {
                                data[2] = 0x10;
                                exponent++;
                                break;
                            }
                            if ((data[index >> 1] >> ((~index & 1) << 2) & 0xF) < 9) {
                                data[index >> 1] += 1 << ((~index & 1) << 2);
                                break;
                            }
                            data[index >> 1] &= 0xF << ((index & 1) << 2);
                        }
                    }
                    index = (dataByteCount << 1) + 1;
                }
            } else if (c == 'e') {
                bool sign = parseSign(i, e);
                int offset = 0;
                do {
                    char c = *i++;
                    if (c >= '0' && c <= '9') {
                        offset *= 10;
                        offset += c - '0';
                    } else {
                        throw std::invalid_argument("Unexpected character.");
                    }
                } while (i != e);
                exponent = sign ? exponent - offset : exponent + offset;
            } else {
                throw std::invalid_argument("Unexpected character.");
            }
        } while (i != e);
        if (noDigits) {
            throw std::invalid_argument("No digits found.");
        }
        if (exponent < 0x80 - 99 || exponent > 0x80 + 99) {
            throw std::invalid_argument("Exponent out of range.");
        }
        data[1] = exponent;
        return data;
    }

    std::string STH_FP::makeStringFromData(const data_t& data, const options_t& options)
    {
        bool scientific = false;
        (void)options;

        if (data.size() != dataByteCount)
        {
            throw std::invalid_argument("Invalid data array. Needs to contain " + std::to_string(dataByteCount) + " bytes");
        }

        bool negative = ((data[0] & 0x80) == 0x80);
        if (!data[2]) {
            return scientific ? "0e0" : "0";
        }
        int exponent = data[1] - 0x80;

        unsigned index = 4, point = 4;
        if (exponent < -3 || exponent > 9) {
            scientific = true;
        } else {
            if (exponent < 0) {
                index += exponent;
            }
            point += exponent;
        }
        char result[25], *i = result;
        if (negative) {
            *i++ = '-';
        }
        do {
            char digit = '0' + (index < 4 ? 0 : data[index >> 1] >> ((~index & 1) << 2) & 0xF);
            *i++ = digit <= '9' ? digit : '?';
            if (index == point) {
                *i++ = '.';
            }
        } while (++index < dataByteCount << 1);
        while (--i != result && *i == '0') {
        }
        if (*i == '.') {
            i--;
        }
        if (scientific) {
            *++i = 'e';
            if (exponent < 0) {
                exponent = -exponent;
                *++i = '-';
            }
            bool leading = true;
            for (int factor = 100; factor > 1; factor /= 10) {
                if (!leading || exponent >= factor) {
                    *++i = '0' + exponent / factor;
                    exponent = exponent % factor;
                    leading = false;
                }
            }
            *++i = '0' + exponent;
        }
        *++i = '\0';
        return std::string(result);
    };
}
