#pragma once

#include <atomic>
#include <string>
#include <vector>

struct SearchOutput {
    std::vector<std::wstring> results;
    std::wstring source;
    std::wstring error;
};

SearchOutput RunSearch(const std::wstring& query, const std::wstring& kind, std::atomic<bool>* cancel_flag);
