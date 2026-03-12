#pragma once

#include <Windows.h>

#include <string>

enum class UiIcon {
    Star,
    Settings,
    Search,
    Save,
    Reset,
    Delete,
    App,
    Document,
    Link,
    External,
};

bool InitializeUiAssets();
void ShutdownUiAssets();
bool DrawUiIcon(HDC dc, UiIcon icon, const RECT& rect, BYTE opacity = 255);
bool DrawFavoriteTargetIcon(HDC dc, const std::wstring& kind, const std::wstring& target, const RECT& rect);
