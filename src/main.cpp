#include "windows.hpp"

#include <Windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>

#include <vector>

namespace {
std::vector<std::wstring> GetArguments() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args;

    if (argv != nullptr) {
        for (int index = 0; index < argc; ++index) {
            args.emplace_back(argv[index]);
        }
        LocalFree(argv);
    }

    return args;
}

bool HasArgument(const std::vector<std::wstring>& args, const std::wstring& target) {
    for (const auto& arg : args) {
        if (arg == target) {
            return true;
        }
    }
    return false;
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    const HRESULT co_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool com_initialized = SUCCEEDED(co_result) || co_result == RPC_E_CHANGED_MODE;

    const auto args = GetArguments();
    const bool smoke_test = HasArgument(args, L"--smoke-test");

    FavoriteApp app(instance, smoke_test);
    if (!app.Initialize()) {
        if (com_initialized) {
            CoUninitialize();
        }
        return 1;
    }

    const int exit_code = app.Run();
    if (com_initialized) {
        CoUninitialize();
    }
    return exit_code;
}
