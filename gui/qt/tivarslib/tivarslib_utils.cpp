/*
 * Part of tivars_lib_cpp
 * (C) 2015-2019 Adrien "Adriweb" Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "tivarslib_utils.h"
#include <sstream>
#include <cmath>

namespace tivars
{

bool has_option(const options_t& m, const std::string& element)
{
    return m.find(element) != m.end();
}

unsigned char hexdec(const std::string& str)
{
    return (unsigned char) stoul(str, nullptr, 16);
}

std::string dechex(unsigned char i, bool zeropad)
{
    std::string str = "00";
    sprintf(&str[0], zeropad ? "%02X" : "%X", i);
    return str;
}

std::string strtoupper(const std::string& str)
{
    std::string newStr(str);
    for (char& c : newStr)
    {
        c = (char) toupper(c);
    }
    return newStr;
}

std::vector<std::string> explode(const std::string& str, const std::string& delim)
{
    std::vector<std::string> result;

    size_t last = 0;
    size_t next = 0;
    while ((next = str.find(delim, last)) != std::string::npos)
    {
        result.push_back(str.substr(last, next - last));
        last = next + delim.length();
    }
    result.push_back(str.substr(last));

    return result;
}

std::vector<std::string> explode(const std::string& str, char delim)
{
    return explode(str, std::string(1, delim));
}

// trim from start
std::string ltrim(std::string s, const char* t)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from end
std::string rtrim(std::string s, const char* t)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from both ends
std::string trim(const std::string& s, const char* t)
{
    return ltrim(rtrim(s, t), t);
}

std::string str_repeat(const std::string& str, unsigned int times)
{
    std::string result;
    result.reserve(times * str.length()); // avoid repeated reallocation
    for (unsigned char i = 0; i < times; i++)
    {
        result += str;
    }
    return result;
}

// From http://stackoverflow.com/a/2481126/378298
void ParseCSV(const std::string& csvSource, std::vector<std::vector<std::string>>& lines)
{
    bool inQuote(false);
    bool newLine(false);
    std::string field;
    lines.clear();
    std::vector<std::string> line;

    std::string::const_iterator aChar = csvSource.begin();
    std::string::const_iterator tmp;

    while (aChar != csvSource.end())
    {
        tmp = aChar;
        switch (*aChar)
        {
            case '"':
                newLine = false;
                // Handle weird escaping of quotes ("""" => ")
                if (*(tmp+1) == '"' && *(tmp+2) == '"' && *(tmp+3) == '"') {
                    field.push_back(*aChar);
                    aChar += 3;
                } else {
                    inQuote = !inQuote;
                }
                break;

            case ',':
                newLine = false;
                if (inQuote)
                {
                    field += *aChar;
                }
                else
                {
                    line.push_back(field);
                    field.clear();
                }
                break;

            case '\n':
            case '\r':
                if (inQuote)
                {
                    field += *aChar;
                }
                else
                {
                    if (!newLine)
                    {
                        line.push_back(field);
                        lines.push_back(line);
                        field.clear();
                        line.clear();
                        newLine = true;
                    }
                }
                break;

            default:
                newLine = false;
                field.push_back(*aChar);
                break;
        }

        ++aChar;
    }

    if (!field.empty())
        line.push_back(field);

    if (!line.empty())
        lines.push_back(line);
}

bool is_numeric(const std::string& str)
{
    char* p;
    double ignored = ::strtod(str.c_str(), &p);
    (void)ignored;
    return (bool)!*p;
}

bool file_exists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (FILE *file = fopen(path.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

std::string str_pad(const std::string& str, unsigned long pad_length, std::string pad_string)
{
    unsigned long i, j, x;
    unsigned long str_size = str.size();
    unsigned long pad_size = pad_string.size();

    if (pad_length <= str_size || pad_size < 1)
    {
        return str;
    }

    std::string o;
    o.reserve(pad_length);

    for (i = 0, x = str_size; i < x; i++)
    {
        o.push_back(str[i]);
    }
    for (i = str_size; i < pad_length;)
    {
        for (j = 0; j < pad_size && i < pad_length; j++, i++)
        {
            o.push_back(pad_string[j]);
        }
    }

    return o;
}

std::string multiple(int num, const std::string &var) {
    const std::string unit = var.empty() ? "1" : var;
    switch (num) {
        case 0:
            return "0";
        case 1:
            return unit;
        case -1:
            return "-" + unit;
        default:
            return std::to_string(num) + var;
    }
}

// Adapted from http://stackoverflow.com/a/32903747/378298
std::string dec2frac(double num, const std::string& var, double err)
{
    if (err <= 0.0 || err >= 1.0)
    {
        err = 0.001;
    }

    int sign = ( num > 0 ) ? 1 : ( ( num < 0 ) ? -1 : 0 );

    if (sign == -1)
    {
        num = std::abs(num);
    }

    if (sign != 0)
    {
        // err is the maximum relative err; convert to absolute
        err *= num;
    }

    int n = (int) std::floor(num);
    num -= n;

    if (num < err)
    {
        return multiple(sign * n, var);
    }

    if (1 - err < num)
    {
        return multiple(sign * (n + 1), var);
    }

    // The lower fraction is 0/1
    int lower_n = 0;
    int lower_d = 1;

    // The upper fraction is 1/1
    int upper_n = 1;
    int upper_d = 1;

    while (true)
    {
        // The middle fraction is (lower_n + upper_n) / (lower_d + upper_d)
        const int middle_n = lower_n + upper_n;
        const int middle_d = lower_d + upper_d;

        if (middle_d * (num + err) < middle_n)
        {
            // real + err < middle : middle is our new upper
            upper_n = middle_n;
            upper_d = middle_d;
        }
        else if (middle_n < (num - err) * middle_d)
        {
            // middle < real - err : middle is our new lower
            lower_n = middle_n;
            lower_d = middle_d;
        } else {
            // Middle is our best fraction
            return multiple((n * middle_d + middle_n) * sign, var) + "/" + std::to_string(middle_d);
        }
    }
}

std::string trimZeros(const std::string& str)
{
    return std::to_string(std::stoi(str));
}

}