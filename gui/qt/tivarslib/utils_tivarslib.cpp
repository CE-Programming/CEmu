/*
 * Part of tivars_lib_cpp
 * (C) 2015-2016 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "utils_tivarslib.h"

using namespace std;

bool is_in_vector_uchar(const std::vector<unsigned char>& v, unsigned char element)
{
    return find(v.begin(), v.end(), element) != v.end();
}

bool is_in_vector_uint(const std::vector<unsigned int>& v, unsigned int element)
{
    return find(v.begin(), v.end(), element) != v.end();
}

bool is_in_vector_string(const std::vector<std::string>& v, std::string element)
{
    return find(v.begin(), v.end(), element) != v.end();
}

bool has_option(const unordered_map<string, unsigned char>& m, const string element)
{
    return m.find(element) != m.end();
}

unsigned char hexdec(const string& str)
{
    return (unsigned char) stoul(str, nullptr, 16);
}

std::string dechex(unsigned char i)
{
    std::stringstream stream;
    stream << std::hex << (unsigned int)i;
    return stream.str();
}

vector<string> explode(const string& str, const string& delim)
{
    vector<string> result;

    size_t last = 0;
    size_t next = 0;
    while ((next = str.find(delim, last)) != string::npos)
    {
        result.push_back(str.substr(last, next - last));
        last = next + delim.length();
    }
    result.push_back(str.substr(last));

    return result;
}

vector<string> explode(const string& str, char delim)
{
    return explode(str, string(1, delim));
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
std::string trim(std::string s, const char* t)
{
    return ltrim(rtrim(s, t), t);
}

string str_repeat(const string& str, unsigned int times)
{
    string result;
    result.reserve(times * str.length()); // avoid repeated reallocation
    for (unsigned char i = 0; i < times; i++)
    {
        result += str;
    }
    return result;
}

// From http://stackoverflow.com/a/2481126/378298
void ParseCSV(const string& csvSource, vector<vector<string>>& lines)
{
    bool inQuote(false);
    bool newLine(false);
    string field;
    lines.clear();
    vector<string> line;

    string::const_iterator aChar = csvSource.begin();
    string::const_iterator tmp;

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

        aChar++;
    }

    if (field.size())
        line.push_back(field);

    if (line.size())
        lines.push_back(line);
}

bool is_numeric(const std::string& str)
{
    char* p;
    double ignored = ::strtod(str.c_str(), &p);
    (void)ignored;
    return (bool)!*p;
}

// From http://rosettacode.org/wiki/Strip_a_set_of_characters_from_a_string#C.2B.2B
std::string stripchars(std::string str, const std::string& chars)
{
    str.erase(std::remove_if(str.begin(), str.end(), [&](char c){ return chars.find(c) != std::string::npos; }), str.end());
    return str;
}

string str_pad(const string& str, unsigned long pad_length, string pad_string)
{
    unsigned long i, j, x;
    unsigned long str_size = str.size();
    unsigned long pad_size = pad_string.size();

    if (pad_length <= str_size || pad_size < 1)
    {
        return str;
    }

    string o;
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

// Adapted from http://stackoverflow.com/a/32903747/378298
string dec2frac(double num, double err)
{
    if (err <= 0.0 || err >= 1.0)
    {
        err = 0.001;
    }

    int sign = ( num > 0 ) ? 1 : ( ( num < 0 ) ? -1 : 0 );

    if (sign == -1)
    {
        num = abs(num);
    }

    if (sign != 0)
    {
        // err is the maximum relative err; convert to absolute
        err *= num;
    }

    int n = (int) floor(num);
    num -= n;

    if (num < err)
    {
        return to_string(sign * n);
    }

    if (1 - err < num)
    {
        return to_string(sign * (n + 1));
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
        int middle_n = lower_n + upper_n;
        int middle_d = lower_d + upper_d;

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
            return to_string((n * middle_d + middle_n) * sign) + "/" + to_string(middle_d);
        }
    }
}

std::string trimZeros(const std::string& str)
{
    return to_string(stoi(str));
}
