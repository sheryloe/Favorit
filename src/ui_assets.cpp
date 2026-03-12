#include "ui_assets.hpp"

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <shlobj.h>

#include <map>
#include <memory>
#include <string>
#include <type_traits>

namespace {
struct IconDeleter {
    void operator()(std::remove_pointer_t<HICON>* icon) const {
        if (icon != nullptr) {
            DestroyIcon(icon);
        }
    }
};

using UniqueIcon = std::unique_ptr<std::remove_pointer_t<HICON>, IconDeleter>;

struct UiAssetsState {
    ULONG_PTR token = 0;
    bool started = false;
    std::wstring icon_directory;
    std::map<UiIcon, std::unique_ptr<Gdiplus::Bitmap>> cache;
    std::map<std::wstring, UniqueIcon> favorite_icon_cache;
};

UiAssetsState& State() {
    static UiAssetsState state;
    return state;
}

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

std::wstring ModuleDirectory() {
    std::wstring buffer(MAX_PATH, L'\0');

    while (true) {
        const DWORD written = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0) {
            return L"";
        }

        if (written < buffer.size() - 1) {
            buffer.resize(written);
            return DirectoryFromPath(buffer);
        }

        buffer.resize(buffer.size() * 2);
    }
}

const wchar_t* IconFileName(UiIcon icon) {
    switch (icon) {
    case UiIcon::Star:
        return L"star.png";
    case UiIcon::Settings:
        return L"settings.png";
    case UiIcon::Search:
        return L"search.png";
    case UiIcon::Save:
        return L"save.png";
    case UiIcon::Reset:
        return L"reset.png";
    case UiIcon::Delete:
        return L"delete.png";
    case UiIcon::App:
        return L"app.png";
    case UiIcon::Document:
        return L"document.png";
    case UiIcon::Link:
        return L"link.png";
    case UiIcon::External:
        return L"external.png";
    }
    return L"";
}

bool EnsureUiAssetsStarted() {
    UiAssetsState& state = State();
    if (state.started) {
        return true;
    }

    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::GdiplusStartup(&state.token, &input, nullptr) != Gdiplus::Ok) {
        return false;
    }

    state.started = true;
    state.icon_directory = JoinPath(JoinPath(ModuleDirectory(), L"assets"), L"icons");
    return true;
}

Gdiplus::Bitmap* LoadIcon(UiIcon icon) {
    if (!EnsureUiAssetsStarted()) {
        return nullptr;
    }

    UiAssetsState& state = State();
    const auto cached = state.cache.find(icon);
    if (cached != state.cache.end()) {
        return cached->second.get();
    }

    auto bitmap = std::make_unique<Gdiplus::Bitmap>(JoinPath(state.icon_directory, IconFileName(icon)).c_str());
    if (bitmap->GetLastStatus() != Gdiplus::Ok) {
        state.cache.emplace(icon, nullptr);
        return nullptr;
    }

    Gdiplus::Bitmap* raw = bitmap.get();
    state.cache.emplace(icon, std::move(bitmap));
    return raw;
}

UniqueIcon ExtractExecutableIcon(const std::wstring& target) {
    HICON large_icon = nullptr;
    HICON small_icon = nullptr;
    const UINT count = ExtractIconExW(target.c_str(), 0, &large_icon, &small_icon, 1);
    if (count > 0) {
        if (large_icon != nullptr) {
            if (small_icon != nullptr) {
                DestroyIcon(small_icon);
            }
            return UniqueIcon(large_icon);
        }
        if (small_icon != nullptr) {
            return UniqueIcon(small_icon);
        }
    }
    return UniqueIcon(nullptr);
}

UniqueIcon LoadShellIcon(const std::wstring& target, DWORD attributes, bool use_file_attributes) {
    SHFILEINFOW info{};
    UINT flags = SHGFI_ICON | SHGFI_LARGEICON;
    if (use_file_attributes) {
        flags |= SHGFI_USEFILEATTRIBUTES;
    }

    if (SHGetFileInfoW(target.c_str(), attributes, &info, sizeof(info), flags) == 0) {
        return UniqueIcon(nullptr);
    }

    return UniqueIcon(info.hIcon);
}

std::wstring FavoriteCacheKey(const std::wstring& kind, const std::wstring& target) {
    return kind + L"|" + target;
}

HICON LoadFavoriteIcon(const std::wstring& kind, const std::wstring& target) {
    if (!EnsureUiAssetsStarted()) {
        return nullptr;
    }

    UiAssetsState& state = State();
    const std::wstring key = FavoriteCacheKey(kind, target);
    const auto cached = state.favorite_icon_cache.find(key);
    if (cached != state.favorite_icon_cache.end()) {
        return cached->second.get();
    }

    UniqueIcon icon(nullptr);
    if (kind == L"app" && !target.empty()) {
        icon = ExtractExecutableIcon(target);
        if (!icon) {
            icon = LoadShellIcon(target, 0, false);
        }
    } else if (kind == L"document" && !target.empty()) {
        const DWORD attributes = GetFileAttributesW(target.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES) {
            icon = LoadShellIcon(target, 0, false);
        } else {
            icon = LoadShellIcon(target, FILE_ATTRIBUTE_NORMAL, true);
        }
    }

    HICON raw = icon.get();
    state.favorite_icon_cache.emplace(key, std::move(icon));
    return raw;
}
}  // namespace

bool InitializeUiAssets() {
    return EnsureUiAssetsStarted();
}

void ShutdownUiAssets() {
    UiAssetsState& state = State();
    state.cache.clear();
    state.favorite_icon_cache.clear();

    if (state.started) {
        Gdiplus::GdiplusShutdown(state.token);
        state.token = 0;
        state.started = false;
    }

    state.icon_directory.clear();
}

bool DrawUiIcon(HDC dc, UiIcon icon, const RECT& rect, BYTE opacity) {
    Gdiplus::Bitmap* bitmap = LoadIcon(icon);
    if (bitmap == nullptr || rect.right <= rect.left || rect.bottom <= rect.top) {
        return false;
    }

    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

    const Gdiplus::Rect destination(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    if (opacity == 255) {
        graphics.DrawImage(bitmap, destination);
        return true;
    }

    Gdiplus::ImageAttributes attributes;
    Gdiplus::ColorMatrix matrix = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, static_cast<Gdiplus::REAL>(opacity) / 255.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    };
    attributes.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

    graphics.DrawImage(
        bitmap,
        destination,
        0,
        0,
        bitmap->GetWidth(),
        bitmap->GetHeight(),
        Gdiplus::UnitPixel,
        &attributes);

    return true;
}

bool DrawFavoriteTargetIcon(HDC dc, const std::wstring& kind, const std::wstring& target, const RECT& rect) {
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return false;
    }

    if (kind == L"url") {
        return DrawUiIcon(dc, UiIcon::Link, rect);
    }

    HICON icon = LoadFavoriteIcon(kind, target);
    if (icon == nullptr) {
        if (kind == L"document") {
            return DrawUiIcon(dc, UiIcon::Document, rect);
        }
        return DrawUiIcon(dc, UiIcon::App, rect);
    }

    return DrawIconEx(
               dc,
               rect.left,
               rect.top,
               icon,
               rect.right - rect.left,
               rect.bottom - rect.top,
               0,
               nullptr,
               DI_NORMAL) != FALSE;
}
