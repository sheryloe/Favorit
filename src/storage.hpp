#pragma once

#include "model.hpp"

#include <string>

class Storage {
public:
    Storage();

    AppState Load() const;
    bool Save(const AppState& state, std::wstring* error = nullptr) const;
    const std::wstring& file_path() const;

private:
    std::wstring file_path_;

    bool EnsureDirectory(std::wstring* error) const;
    std::wstring ReadString(const std::wstring& section, const std::wstring& key, const std::wstring& fallback) const;
    int ReadInt(const std::wstring& section, const std::wstring& key, int fallback) const;
    bool WriteString(const std::wstring& section, const std::wstring& key, const std::wstring& value) const;
    bool WriteInt(const std::wstring& section, const std::wstring& key, int value) const;
};
