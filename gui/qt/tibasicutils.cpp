#include "tibasicutils.h"

#include "tivars_lib_cpp/tivars_lib_cpp.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view STORE_ARROW = "\xE2\x86\x92";
constexpr std::string_view TOKEN_BOUNDARY_HAIR_SPACE = "\xE2\x80\x8A";

bool startsWith(const std::string &text, size_t pos, std::string_view value) {
    return pos + value.size() <= text.size() &&
           std::string_view(text).substr(pos, value.size()) == value;
}

bool isFormattingWhitespace(const std::string &line, size_t pos) {
    if (startsWith(line, pos, TOKEN_BOUNDARY_HAIR_SPACE)) {
        return false;
    }
    const char c = line[pos];
    return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}

bool trailingSpaceCompletesToken(const std::string &line) {
    const data_t withoutSpace = tivars::TypeHandlers::TH_Tokenized::makeDataFromString(line);
    const data_t withSpace = tivars::TypeHandlers::TH_Tokenized::makeDataFromString(line + ' ');

    const bool appendedSpaceToken = withSpace.size() == withoutSpace.size() + 1 &&
                                    withSpace.back() == 0x29 &&
                                    std::equal(withoutSpace.begin() + 2, withoutSpace.end(),
                                               withSpace.begin() + 2);
    return !appendedSpaceToken;
}

std::vector<std::string> splitLines(const std::string &source) {
    std::vector<std::string> lines;
    size_t start = 0;
    for (size_t pos = 0; pos < source.size(); pos++) {
        if (source[pos] != '\r' && source[pos] != '\n') {
            continue;
        }
        lines.push_back(source.substr(start, pos - start));
        if (source[pos] == '\r' && pos + 1 < source.size() && source[pos + 1] == '\n') {
            pos++;
        }
        start = pos + 1;
    }
    lines.push_back(source.substr(start));
    return lines;
}

std::string prepareLine(const std::string &line) {
    std::string prepared;
    bool withinString = false;
    bool pendingSpace = false;

    for (size_t pos = 0; pos < line.size(); pos++) {
        if (!withinString && line[pos] == '#') {
            break;
        }

        if (!withinString && isFormattingWhitespace(line, pos)) {
            pendingSpace = !prepared.empty();
            continue;
        }

        if (pendingSpace) {
            prepared.push_back(' ');
            pendingSpace = false;
        }

        if (line[pos] == '"') {
            withinString = !withinString;
            prepared.push_back(line[pos]);
            continue;
        }

        if (withinString && startsWith(line, pos, STORE_ARROW)) {
            prepared.append(STORE_ARROW);
            pos += STORE_ARROW.size() - 1;
            withinString = false;
            continue;
        }
        if (withinString && startsWith(line, pos, "->")) {
            prepared.append("->");
            pos++;
            withinString = false;
            continue;
        }

        prepared.push_back(line[pos]);
    }

    if (pendingSpace && trailingSpaceCompletesToken(prepared)) {
        prepared.push_back(' ');
    }

    return prepared;
}

}

std::string ti_basic_prepare_source(const std::string &source) {
    std::string prepared;
    for (const std::string &line : splitLines(source)) {
        const std::string preparedLine = prepareLine(line);
        if (preparedLine.empty()) {
            continue;
        }
        if (!prepared.empty()) {
            prepared.push_back('\n');
        }
        prepared += preparedLine;
    }
    return prepared;
}

std::string ti_basic_deindent_source(const std::string &source) {
    std::string deindented;
    const std::vector<std::string> lines = splitLines(source);
    for (size_t index = 0; index < lines.size(); index++) {
        const std::string &line = lines[index];
        size_t pos = 0;
        while (pos < line.size() && isFormattingWhitespace(line, pos)) {
            pos++;
        }
        deindented += line.substr(pos);
        if (index + 1 < lines.size()) {
            deindented.push_back('\n');
        }
    }
    return deindented;
}
