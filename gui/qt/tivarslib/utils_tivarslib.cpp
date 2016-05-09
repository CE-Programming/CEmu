/*
 * Part of tivars_lib_cpp
 * (C) 2015 Adrien 'Adriweb' Bertrand
 * https://github.com/adriweb/tivars_lib_cpp
 * License: MIT
 */

#include "utils_tivarslib.h"
#include <sstream>
#include <iomanip>
#include <regex>
#include <functional>

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

bool is_in_umap_string_uchar(const unordered_map<string, unsigned char>& m, const string element)
{
    return m.find(element) != m.end();
}

bool is_in_umap_string_uint(const unordered_map<string, unsigned int>& m, const string element)
{
    return m.find(element) != m.end();
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

vector<string> explode(const string& str, char delim)
{
    vector<string> result;
    istringstream iss(str);

    for (string token; getline(iss, token, delim);)
    {
        result.push_back(move(token));
    }

    return result;
}

// trim from start
string& ltrim(string& s)
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    return s;
}

// trim from end
string& rtrim(string& s)
{
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// trim from both ends
string& trim(string& s)
{
    return ltrim(rtrim(s));
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
    return std::regex_match(str, std::regex("[(-|+)|][0-9]*\\.?[0-9]+"));
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
