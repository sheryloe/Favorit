#pragma once

#include <string>
#include <vector>

std::wstring Trim(const std::wstring& value);
std::wstring ToLower(const std::wstring& value);
bool IStartsWith(const std::wstring& value, const std::wstring& prefix);
std::wstring Utf8ToWide(const std::string& value);
std::string WideToUtf8(const std::wstring& value);
std::wstring GetEnvValue(const wchar_t* name);
std::wstring GenerateId();
std::vector<std::wstring> SplitLines(const std::wstring& text);
