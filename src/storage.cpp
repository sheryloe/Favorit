#include "storage.hpp"

#include "utils.hpp"

#include <Windows.h>
#include <shlobj.h>

namespace {
std::wstring JoinPath(const std::wstring& base, const std::wstring& child) {
    if (base.empty()) {
        return child;
    }
    if (base.back() == L'\\' || base.back() == L'/') {
        return base + child;
    }
    return base + L"\\" + child;
}

std::wstring DirectoryFromPath(const std::wstring& path) {
    const std::wstring::size_type slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, slash);
}

std::wstring DefaultStoragePath() {
    std::wstring base = GetEnvValue(L"LOCALAPPDATA");
    if (base.empty()) {
        base = GetEnvValue(L"USERPROFILE");
    }
    return JoinPath(JoinPath(base, L"FavoriteWidget"), L"favorite-widget.ini");
}
}  // namespace

Storage::Storage() : file_path_(DefaultStoragePath()) {}

AppState Storage::Load() const {
    AppState state;

    const DWORD attributes = GetFileAttributesW(file_path_.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return state;
    }

    state.widget.x = ReadInt(L"Widget", L"X", 48);
    state.widget.y = ReadInt(L"Widget", L"Y", 48);

    const int count = ReadInt(L"Favorites", L"Count", 0);
    for (int index = 0; index < count; ++index) {
        const std::wstring section = L"Favorite" + std::to_wstring(index);
        FavoriteItem item;
        item.id = ReadString(section, L"Id", L"");
        item.label = ReadString(section, L"Label", L"");
        item.kind = ReadString(section, L"Kind", L"");
        item.target = ReadString(section, L"Target", L"");

        if (!item.id.empty() && !item.label.empty() && !item.kind.empty() && !item.target.empty()) {
            state.favorites.push_back(item);
        }
    }

    return state;
}

bool Storage::Save(const AppState& state, std::wstring* error) const {
    if (!EnsureDirectory(error)) {
        return false;
    }

    const int previous_count = ReadInt(L"Favorites", L"Count", 0);
    for (int index = 0; index < previous_count; ++index) {
        const std::wstring section = L"Favorite" + std::to_wstring(index);
        WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, file_path_.c_str());
    }

    if (!WriteInt(L"Widget", L"X", state.widget.x) || !WriteInt(L"Widget", L"Y", state.widget.y)) {
        if (error != nullptr) {
            *error = L"\uC704\uC82F \uC704\uCE58\uB97C \uC800\uC7A5\uD558\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.";
        }
        return false;
    }

    if (!WriteInt(L"Favorites", L"Count", static_cast<int>(state.favorites.size()))) {
        if (error != nullptr) {
            *error = L"\uD56D\uBAA9 \uAC1C\uC218\uB97C \uC800\uC7A5\uD558\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.";
        }
        return false;
    }

    for (std::size_t index = 0; index < state.favorites.size(); ++index) {
        const std::wstring section = L"Favorite" + std::to_wstring(index);
        const FavoriteItem& item = state.favorites[index];

        if (!WriteString(section, L"Id", item.id) ||
            !WriteString(section, L"Label", item.label) ||
            !WriteString(section, L"Kind", item.kind) ||
            !WriteString(section, L"Target", item.target)) {
            if (error != nullptr) {
                *error = L"\uD56D\uBAA9\uC744 \uC800\uC7A5\uD558\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.";
            }
            return false;
        }
    }

    return true;
}

const std::wstring& Storage::file_path() const {
    return file_path_;
}

bool Storage::EnsureDirectory(std::wstring* error) const {
    const std::wstring directory = DirectoryFromPath(file_path_);
    if (directory.empty()) {
        return true;
    }

    const int result = SHCreateDirectoryExW(nullptr, directory.c_str(), nullptr);
    if (result != ERROR_SUCCESS && result != ERROR_FILE_EXISTS && result != ERROR_ALREADY_EXISTS) {
        if (error != nullptr) {
            *error = L"\uC124\uC815 \uD3F4\uB354\uB97C \uB9CC\uB4E4\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.";
        }
        return false;
    }
    return true;
}

std::wstring Storage::ReadString(const std::wstring& section, const std::wstring& key, const std::wstring& fallback) const {
    std::wstring buffer(4096, L'\0');
    const DWORD count = GetPrivateProfileStringW(
        section.c_str(),
        key.c_str(),
        fallback.c_str(),
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        file_path_.c_str()
    );
    buffer.resize(count);
    return buffer;
}

int Storage::ReadInt(const std::wstring& section, const std::wstring& key, int fallback) const {
    return static_cast<int>(GetPrivateProfileIntW(section.c_str(), key.c_str(), fallback, file_path_.c_str()));
}

bool Storage::WriteString(const std::wstring& section, const std::wstring& key, const std::wstring& value) const {
    return WritePrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), file_path_.c_str()) != FALSE;
}

bool Storage::WriteInt(const std::wstring& section, const std::wstring& key, int value) const {
    return WriteString(section, key, std::to_wstring(value));
}
