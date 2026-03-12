#pragma once

#include <string>
#include <vector>

struct FavoriteItem {
    std::wstring id;
    std::wstring label;
    std::wstring kind;
    std::wstring target;
};

struct WidgetState {
    int x = 48;
    int y = 48;
};

struct AppState {
    WidgetState widget;
    std::vector<FavoriteItem> favorites;
};
