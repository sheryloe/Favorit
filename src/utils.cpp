#include "utils.hpp"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <objbase.h>
#include <sstream>

std::wstring Trim(const std::wstring& value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) {
        return L"";
    }

    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::wstring ToLower(const std::wstring& value) {
    std::wstring lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return lowered;
}

bool IStartsWith(const std::wstring& value, const std::wstring& prefix) {
    if (prefix.size() > value.size()) {
        return false;
    }
    return ToLower(value.substr(0, prefix.size())) == ToLower(prefix);
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return L"";
    }

    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (wide_length > 0) {
        std::wstring wide(static_cast<std::size_t>(wide_length - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wide.data(), wide_length);
        return wide;
    }

    const int ansi_length = MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(ansi_length - 1), L'\0');
    MultiByteToWideChar(CP_ACP, 0, value.c_str(), -1, wide.data(), ansi_length);
    return wide;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return "";
    }

    const int utf8_length = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(utf8_length - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, utf8.data(), utf8_length, nullptr, nullptr);
    return utf8;
}

std::wstring GetEnvValue(const wchar_t* name) {
    const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) {
        return L"";
    }

    std::wstring buffer(static_cast<std::size_t>(required), L'\0');
    GetEnvironmentVariableW(name, buffer.data(), required);
    buffer.resize(static_cast<std::size_t>(required - 1));
    return buffer;
}

std::wstring GenerateId() {
    GUID guid{};
    if (CoCreateGuid(&guid) != S_OK) {
        return L"generated-id";
    }

    wchar_t buffer[64]{};
    StringFromGUID2(guid, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    return buffer;
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    std::wstringstream stream(text);
    std::wstring line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    return lines;
}
