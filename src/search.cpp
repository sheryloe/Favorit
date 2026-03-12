#include "search.hpp"

#include "utils.hpp"

#include <Windows.h>

#include <set>
#include <string>
#include <vector>

namespace {
constexpr std::size_t kMaxResults = 80;

std::wstring JoinPath(const std::wstring& base, const std::wstring& child) {
    if (base.empty()) {
        return child;
    }
    if (base.back() == L'\\' || base.back() == L'/') {
        return base + child;
    }
    return base + L"\\" + child;
}

std::wstring FileNameFromPath(const std::wstring& path) {
    const std::wstring::size_type slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

std::wstring FileExtensionFromPath(const std::wstring& path) {
    const std::wstring name = FileNameFromPath(path);
    const std::wstring::size_type dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos) {
        return L"";
    }
    return ToLower(name.substr(dot));
}

bool PathExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool IsRegularFile(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring QuoteArgument(const std::wstring& value) {
    if (value.find(L' ') == std::wstring::npos && value.find(L'"') == std::wstring::npos) {
        return value;
    }

    std::wstring escaped = L"\"";
    for (wchar_t ch : value) {
        if (ch == L'"') {
            escaped += L"\\\"";
        } else {
            escaped.push_back(ch);
        }
    }
    escaped += L"\"";
    return escaped;
}

std::wstring FindEverythingCli() {
    wchar_t buffer[MAX_PATH]{};
    if (SearchPathW(nullptr, L"es.exe", nullptr, MAX_PATH, buffer, nullptr) > 0) {
        return buffer;
    }

    const std::vector<std::wstring> candidates = {
        L"C:\\Program Files\\Everything\\es.exe",
        L"C:\\Program Files (x86)\\Everything\\es.exe",
    };

    for (const auto& candidate : candidates) {
        if (PathExists(candidate)) {
            return candidate;
        }
    }

    return L"";
}

std::wstring CaptureProcessOutput(const std::wstring& command_line, std::atomic<bool>* cancel_flag) {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;

    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    if (!CreatePipe(&read_pipe, &write_pipe, &attributes, 0)) {
        return L"";
    }

    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = write_pipe;
    startup.hStdError = write_pipe;

    PROCESS_INFORMATION process{};
    std::wstring mutable_command = command_line;
    const BOOL created = CreateProcessW(
        nullptr,
        mutable_command.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startup,
        &process
    );

    CloseHandle(write_pipe);
    write_pipe = nullptr;

    if (!created) {
        CloseHandle(read_pipe);
        return L"";
    }

    std::string output;
    char buffer[4096];
    DWORD bytes_read = 0;
    while (ReadFile(read_pipe, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read > 0) {
        output.append(buffer, buffer + bytes_read);
        if (cancel_flag != nullptr && cancel_flag->load()) {
            TerminateProcess(process.hProcess, 1);
            break;
        }
    }

    WaitForSingleObject(process.hProcess, 5000);

    CloseHandle(read_pipe);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    return Utf8ToWide(output);
}

bool MatchesKind(const std::wstring& kind, const std::wstring& path) {
    const std::wstring extension = FileExtensionFromPath(path);
    if (kind == L"app") {
        return extension == L".exe";
    }
    return extension != L".exe";
}

std::vector<std::wstring> SearchRoots(const std::wstring& kind) {
    const std::wstring home = GetEnvValue(L"USERPROFILE");
    const std::wstring local_app_data = GetEnvValue(L"LOCALAPPDATA");
    const std::wstring program_files = GetEnvValue(L"ProgramFiles");
    const std::wstring program_files_x86 = GetEnvValue(L"ProgramFiles(x86)");
    const std::wstring one_drive = JoinPath(home, L"OneDrive");

    std::vector<std::wstring> roots;
    if (kind == L"app") {
        roots = {
            program_files,
            program_files_x86,
            JoinPath(local_app_data, L"Programs"),
            JoinPath(home, L"Desktop"),
        };
    } else {
        roots = {
            JoinPath(home, L"Desktop"),
            JoinPath(home, L"Documents"),
            JoinPath(home, L"Downloads"),
            JoinPath(one_drive, L"Desktop"),
            JoinPath(one_drive, L"Documents"),
        };
    }

    std::vector<std::wstring> existing;
    for (const auto& root : roots) {
        const DWORD attributes = GetFileAttributesW(root.c_str());
        if (!root.empty() && attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            existing.push_back(root);
        }
    }

    return existing;
}

bool ShouldSkipDirectory(const std::wstring& path) {
    static const std::set<std::wstring> skipped = {
        L".git",
        L".venv",
        L"node_modules",
        L"__pycache__",
        L"temp",
        L"windowsapps",
        L"$recycle.bin",
    };

    const std::wstring name = ToLower(FileNameFromPath(path));
    return skipped.find(name) != skipped.end();
}

std::vector<std::wstring> SearchWithEverything(const std::wstring& query, const std::wstring& kind, std::atomic<bool>* cancel_flag) {
    const std::wstring cli = FindEverythingCli();
    if (cli.empty()) {
        return {};
    }

    const std::wstring command = QuoteArgument(cli) + L" -n 240 " + QuoteArgument(query);
    const std::wstring output = CaptureProcessOutput(command, cancel_flag);
    if (output.empty()) {
        return {};
    }

    std::vector<std::wstring> results;
    std::set<std::wstring> seen;
    for (const auto& line : SplitLines(output)) {
        if (cancel_flag != nullptr && cancel_flag->load()) {
            break;
        }

        const std::wstring trimmed = Trim(line);
        if (trimmed.empty()) {
            continue;
        }

        if (!IsRegularFile(trimmed) || !MatchesKind(kind, trimmed)) {
            continue;
        }

        if (seen.insert(trimmed).second) {
            results.push_back(trimmed);
        }

        if (results.size() >= kMaxResults) {
            break;
        }
    }

    return results;
}

std::vector<std::wstring> SearchLocally(const std::wstring& query, const std::wstring& kind, std::atomic<bool>* cancel_flag) {
    const std::wstring lowered_query = ToLower(query);
    std::vector<std::wstring> results;
    std::set<std::wstring> seen;

    for (const auto& root : SearchRoots(kind)) {
        std::vector<std::wstring> stack{root};

        while (!stack.empty()) {
            if (cancel_flag != nullptr && cancel_flag->load()) {
                return results;
            }

            const std::wstring current = stack.back();
            stack.pop_back();

            WIN32_FIND_DATAW data{};
            HANDLE handle = FindFirstFileW(JoinPath(current, L"*").c_str(), &data);
            if (handle == INVALID_HANDLE_VALUE) {
                continue;
            }

            do {
                if (cancel_flag != nullptr && cancel_flag->load()) {
                    FindClose(handle);
                    return results;
                }

                const std::wstring name = data.cFileName;
                if (name == L"." || name == L"..") {
                    continue;
                }

                const std::wstring full_path = JoinPath(current, name);
                const bool is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                const bool is_reparse_point = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

                if (is_directory) {
                    if (!is_reparse_point && !ShouldSkipDirectory(full_path)) {
                        stack.push_back(full_path);
                    }
                    continue;
                }

                const std::wstring lowered_name = ToLower(name);
                if (lowered_name.find(lowered_query) != std::wstring::npos && MatchesKind(kind, full_path)) {
                    if (seen.insert(full_path).second) {
                        results.push_back(full_path);
                    }

                    if (results.size() >= kMaxResults) {
                        FindClose(handle);
                        return results;
                    }
                }
            } while (FindNextFileW(handle, &data) != FALSE);

            FindClose(handle);
        }
    }

    return results;
}
}  // namespace

SearchOutput RunSearch(const std::wstring& query, const std::wstring& kind, std::atomic<bool>* cancel_flag) {
    SearchOutput output;

    const std::wstring trimmed = Trim(query);
    if (trimmed.empty()) {
        output.error = L"\uAC80\uC0C9\uC5B4\uB97C \uC785\uB825\uD558\uC138\uC694.";
        return output;
    }

    output.results = SearchWithEverything(trimmed, kind, cancel_flag);
    if (!output.results.empty()) {
        output.source = L"Everything";
        return output;
    }

    output.results = SearchLocally(trimmed, kind, cancel_flag);
    output.source = L"\uB85C\uCEEC \uAC80\uC0C9";
    return output;
}
