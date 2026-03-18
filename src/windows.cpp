#include "windows.hpp"

#include "search.hpp"
#include "ui_assets.hpp"
#include "utils.hpp"

#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <windowsx.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr UINT kMessageSearchComplete = WM_APP + 1;
constexpr UINT kMessageMoveFavorite = WM_APP + 2;

constexpr int kIdSettingsButton = 100;
constexpr int kIdExitMenu = 101;
constexpr int kIdCloseButton = 102;
constexpr int kIdFavoriteButtonBase = 1000;

constexpr int kIdListFavorites = 200;
constexpr int kIdEditLabel = 201;
constexpr int kIdComboKind = 202;
constexpr int kIdEditTarget = 203;
constexpr int kIdButtonSearch = 204;
constexpr int kIdButtonSave = 205;
constexpr int kIdButtonReset = 206;
constexpr int kIdButtonDelete = 207;
constexpr int kIdButtonMoveUp = 208;
constexpr int kIdButtonMoveDown = 209;
constexpr int kIdButtonBackup = 210;
constexpr int kIdButtonRestore = 211;

constexpr int kIdSearchQuery = 300;
constexpr int kIdSearchButton = 301;
constexpr int kIdSearchResults = 302;
constexpr int kIdSearchSelect = 303;
constexpr int kIdSearchCancel = 304;

constexpr int kIdGlobalHotkey = 9000;

constexpr COLORREF kWidgetBackground = RGB(10, 12, 14);
constexpr COLORREF kWidgetBackgroundBottom = RGB(10, 12, 14);
constexpr COLORREF kWindowBackground = RGB(10, 12, 14);
constexpr COLORREF kWindowBackgroundBottom = RGB(10, 12, 14);
constexpr COLORREF kPanelBackground = RGB(16, 20, 24);
constexpr COLORREF kPanelElevated = RGB(22, 28, 34);
constexpr COLORREF kInputBackground = RGB(6, 8, 10);
constexpr COLORREF kBorderColor = RGB(0, 120, 160);
constexpr COLORREF kAccentColor = RGB(0, 240, 255);
constexpr COLORREF kAccentColorPressed = RGB(0, 190, 210);
constexpr COLORREF kAccentColorSoft = RGB(0, 60, 80);
constexpr COLORREF kAccentGlow = RGB(100, 255, 255);
constexpr COLORREF kDangerColor = RGB(255, 60, 90);
constexpr COLORREF kDangerColorPressed = RGB(200, 40, 70);
constexpr COLORREF kTextPrimary = RGB(210, 240, 255);
constexpr COLORREF kTextMuted = RGB(90, 140, 160);
constexpr COLORREF kTextDanger = RGB(255, 100, 120);
constexpr COLORREF kWidgetText = kTextPrimary;

const wchar_t* kTitleText = L"\uD398\uC774\uBCF4\uB9BF";
const wchar_t* kWidgetSubtitleText = L"\uBE60\uB978 \uC2E4\uD589 \uC704\uC82F";
const wchar_t* kSettingsTitleText = L"\uD398\uC774\uBCF4\uB9BF \uC124\uC815";
const wchar_t* kSearchTitleText = L"\uD398\uC774\uBCF4\uB9BF \uAC80\uC0C9";
const wchar_t* kEmptyStateTitle = L"\uC544\uC9C1 \uB4F1\uB85D\uB41C \uD56D\uBAA9\uC774 \uC5C6\uC5B4\uC694.";
const wchar_t* kEmptyStateSubtitle = L"\uC124\uC815\uC5D0\uC11C \uC990\uACA8\uCC3E\uAE30\uB97C \uCD94\uAC00\uD574 \uBCF4\uC138\uC694.";

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMSBT_MAINWINDOW
#define DWMSBT_MAINWINDOW 2
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif

HFONT CreateUiFont(int height, int weight = FW_NORMAL, const wchar_t* face = L"Segoe UI Variable Text") {
    return CreateFontW(
        -height,
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        face);
}

void ApplyFont(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

HBRUSH WidgetBrush() {
    static HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    return brush;
}

HBRUSH WindowBrush() {
    static HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    return brush;
}

HBRUSH PanelBrush() {
    static HBRUSH brush = CreateSolidBrush(kPanelBackground);
    return brush;
}

HBRUSH ElevatedBrush() {
    static HBRUSH brush = CreateSolidBrush(kPanelElevated);
    return brush;
}

HBRUSH InputBrush() {
    static HBRUSH brush = CreateSolidBrush(kInputBackground);
    return brush;
}

HBRUSH HeaderBrush() {
    static HBRUSH brush = CreateSolidBrush(ScaleColor(kPanelBackground, 1.01));
    return brush;
}

HBRUSH GridBrush() {
    static HBRUSH brush = CreateSolidBrush(ScaleColor(kPanelBackground, 0.98));
    return brush;
}

COLORREF ScaleColor(COLORREF color, double factor) {
    const int r = std::clamp(static_cast<int>(GetRValue(color) * factor), 0, 255);
    const int g = std::clamp(static_cast<int>(GetGValue(color) * factor), 0, 255);
    const int b = std::clamp(static_cast<int>(GetBValue(color) * factor), 0, 255);
    return RGB(r, g, b);
}

void FillVerticalGradient(HDC dc, const RECT& rect, COLORREF top, COLORREF bottom) {
    TRIVERTEX vertices[2] = {
        {rect.left, rect.top, static_cast<COLOR16>(GetRValue(top) << 8), static_cast<COLOR16>(GetGValue(top) << 8), static_cast<COLOR16>(GetBValue(top) << 8), 0x0000},
        {rect.right, rect.bottom, static_cast<COLOR16>(GetRValue(bottom) << 8), static_cast<COLOR16>(GetGValue(bottom) << 8), static_cast<COLOR16>(GetBValue(bottom) << 8), 0x0000},
    };
    GRADIENT_RECT gradient_rect{0, 1};
    GradientFill(dc, vertices, 2, &gradient_rect, 1, GRADIENT_FILL_RECT_V);
}

void DrawRoundedRect(HDC dc, const RECT& rect, COLORREF fill, COLORREF border, int radius) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    const HGDIOBJ old_brush = SelectObject(dc, brush);
    const HGDIOBJ old_pen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
}

enum class ButtonTone {
    Neutral,
    Accent,
    Danger,
    Tile,
};

enum class TileKind : LONG_PTR {
    None = 0,
    App = 1,
    Document = 2,
    Url = 3,
    Divider = 4,
};

struct FavoriteVisualData {
    std::wstring kind;
    std::wstring target;
};

std::unordered_map<HWND, FavoriteVisualData>& FavoriteVisualStore() {
    static std::unordered_map<HWND, FavoriteVisualData> store;
    return store;
}

void RememberFavoriteVisual(HWND hwnd, const FavoriteItem& favorite) {
    FavoriteVisualStore()[hwnd] = FavoriteVisualData{favorite.kind, favorite.target};
}

void ForgetFavoriteVisual(HWND hwnd) {
    FavoriteVisualStore().erase(hwnd);
}

const FavoriteVisualData* FavoriteVisualFor(HWND hwnd) {
    const auto it = FavoriteVisualStore().find(hwnd);
    if (it == FavoriteVisualStore().end()) {
        return nullptr;
    }
    return &it->second;
}

std::wstring KindDisplayName(const std::wstring& kind);
std::wstring FileNameFromPath(const std::wstring& path);

ButtonTone ToneForButtonId(int control_id) {
    if (control_id == kIdButtonDelete) {
        return ButtonTone::Danger;
    }
    if (control_id == kIdButtonSave || control_id == kIdSearchSelect || control_id == kIdSearchButton || control_id >= kIdFavoriteButtonBase) {
        return control_id >= kIdFavoriteButtonBase ? ButtonTone::Tile : ButtonTone::Accent;
    }
    return ButtonTone::Neutral;
}

TileKind TileKindFromString(const std::wstring& kind) {
    if (kind == L"document") {
        return TileKind::Document;
    }
    if (kind == L"url") {
        return TileKind::Url;
    }
    if (kind == L"app") {
        return TileKind::App;
    }
    if (kind == L"divider") {
        return TileKind::Divider;
    }
    return TileKind::None;
}

UiIcon IconForTileKind(TileKind kind) {
    switch (kind) {
    case TileKind::Document:
        return UiIcon::Document;
    case TileKind::Url:
        return UiIcon::Link;
    case TileKind::Divider:
        return UiIcon::App; // 사용되지 않음
    case TileKind::App:
    case TileKind::None:
        return UiIcon::App;
    }
    return UiIcon::App;
}

std::wstring TileKindActionText(TileKind kind) {
    switch (kind) {
    case TileKind::Document:
        return L"\uBB38\uC11C \uC5F4\uAE30";
    case TileKind::Url:
        return L"\uC6F9 \uB9C1\uD06C \uC5F4\uAE30";
    case TileKind::App:
    case TileKind::None:
        return L"\uD504\uB85C\uADF8\uB7A8 \uC2E4\uD589";
    }
    return L"";
}

UiIcon IconForButtonId(int control_id, LONG_PTR user_data) {
    if (control_id >= kIdFavoriteButtonBase) {
        return IconForTileKind(static_cast<TileKind>(user_data));
    }

    switch (control_id) {
    case kIdSettingsButton:
        return UiIcon::Settings;
    case kIdButtonSearch:
    case kIdSearchButton:
        return UiIcon::Search;
    case kIdButtonSave:
        return UiIcon::Save;
    case kIdButtonReset:
        return UiIcon::Reset;
    case kIdButtonDelete:
        return UiIcon::Delete;
    case kIdSearchSelect:
        return UiIcon::External;
    default:
        return UiIcon::Star;
    }
}

bool HasButtonIcon(int control_id) {
    return control_id == kIdSettingsButton ||
           control_id == kIdButtonSearch ||
           control_id == kIdButtonSave ||
           control_id == kIdButtonReset ||
           control_id == kIdButtonDelete ||
           control_id == kIdSearchButton ||
           control_id == kIdSearchSelect ||
           control_id >= kIdFavoriteButtonBase;
}

bool IconOnlyButton(int control_id) {
    return control_id == kIdSettingsButton;
}

std::wstring ReadControlText(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring value(static_cast<std::size_t>(length + 1), L'\0');
    GetWindowTextW(hwnd, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

RECT InsetRectCopy(const RECT& rect, int x, int y) {
    RECT inset = rect;
    InflateRect(&inset, -x, -y);
    return inset;
}

void DrawTextBlock(HDC dc, const RECT& rect, const std::wstring& text, HFONT font, COLORREF color, UINT format) {
    const HGDIOBJ old_font = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    RECT text_rect = rect;
    DrawTextW(dc, text.c_str(), -1, &text_rect, format | DT_END_ELLIPSIS);
    SelectObject(dc, old_font);
}

void DrawIconBadge(HDC dc, const RECT& rect, COLORREF fill, COLORREF border, UiIcon icon, BYTE opacity = 255) {
    DrawRoundedRect(dc, rect, fill, border, 14);
    RECT icon_rect = InsetRectCopy(rect, 8, 8);
    DrawUiIcon(dc, icon, icon_rect, opacity);
}

class DoubleBuffer {
public:
    DoubleBuffer(HDC target, const RECT& rect) : target_(target), rect_(rect) {
        width_ = rect_.right - rect_.left;
        height_ = rect_.bottom - rect_.top;
        mem_ = CreateCompatibleDC(target_);
        bmp_ = CreateCompatibleBitmap(target_, width_, height_);
        old_bmp_ = SelectObject(mem_, bmp_);
        SetWindowOrgEx(mem_, rect_.left, rect_.top, &old_org_);

        // 둥근 모서리 바깥쪽 배경을 유지하기 위해 기존 화면을 메모리로 먼저 복사
        BitBlt(mem_, rect_.left, rect_.top, width_, height_, target_, rect_.left, rect_.top, SRCCOPY);
    }
    ~DoubleBuffer() {
        // 완성된 버퍼를 화면에 한 번에 출력 (깜빡임 방지)
        BitBlt(target_, rect_.left, rect_.top, width_, height_, mem_, rect_.left, rect_.top, SRCCOPY);
        SetWindowOrgEx(mem_, old_org_.x, old_org_.y, nullptr);
        SelectObject(mem_, old_bmp_);
        DeleteObject(bmp_);
        DeleteDC(mem_);
    }
    HDC hdc() const { return mem_; }
private:
    HDC target_, mem_;
    RECT rect_;
    int width_, height_;
    HBITMAP bmp_;
    HGDIOBJ old_bmp_;
    POINT old_org_;
};

void DrawFavoriteListButton(const DRAWITEMSTRUCT* item, HFONT title_font, HFONT detail_font) {
    DoubleBuffer buffer(item->hDC, item->rcItem);
    HDC dc = buffer.hdc();
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    const bool focused = (item->itemState & ODS_FOCUS) != 0;
    const FavoriteVisualData* visual = FavoriteVisualFor(item->hwndItem);

    bool is_divider = visual != nullptr && visual->kind == L"divider";
    if (is_divider) {
        RECT text_rect = item->rcItem;
        text_rect.left += 12;
        
        const std::wstring label = ReadControlText(item->hwndItem);
        SIZE text_size{0, 0};
        if (!label.empty()) {
            const HGDIOBJ old_font = SelectObject(dc, detail_font);
            GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.length()), &text_size);
            SelectObject(dc, old_font);
            DrawTextBlock(dc, text_rect, label, detail_font, kAccentColor, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        RECT line_rect = item->rcItem;
        line_rect.left = text_rect.left + text_size.cx + (label.empty() ? 0 : 8);
        line_rect.right -= 12;
        line_rect.top = line_rect.top + (line_rect.bottom - line_rect.top) / 2;
        line_rect.bottom = line_rect.top + 1;
        DrawRoundedRect(dc, line_rect, ScaleColor(kBorderColor, 0.5), ScaleColor(kBorderColor, 0.5), 0);
        return;
    }

    COLORREF fill = pressed ? ScaleColor(kPanelElevated, 1.06) : kPanelBackground;
    COLORREF border = focused || pressed ? kAccentColor : ScaleColor(kBorderColor, 1.02);
    COLORREF text = disabled ? kTextMuted : kTextPrimary;

    RECT face_rect = item->rcItem;
    if (pressed) {
        OffsetRect(&face_rect, 0, 1);
    }
    DrawRoundedRect(dc, face_rect, fill, border, 0);

    // 사이버펑크 느낌의 좌측 액센트 표시기
    RECT left_rule{face_rect.left + 4, face_rect.top + 8, face_rect.left + 6, face_rect.bottom - 8};
    DrawRoundedRect(dc, left_rule, focused || pressed ? kAccentGlow : kAccentColor, focused || pressed ? kAccentGlow : kAccentColor, 0);

    const int icon_size = 20;
    const RECT icon_rect{
        face_rect.left + 16,
        face_rect.top + (face_rect.bottom - face_rect.top - icon_size) / 2,
        face_rect.left + 16 + icon_size,
        face_rect.top + (face_rect.bottom - face_rect.top + icon_size) / 2
    };
    
    if (visual == nullptr || !DrawFavoriteTargetIcon(dc, visual->kind, visual->target, icon_rect)) {
        DrawUiIcon(dc, UiIcon::App, icon_rect, disabled ? 170 : 255);
    }

    const int index = static_cast<int>(item->CtlID) - kIdFavoriteButtonBase;
    int text_left = icon_rect.right + 12;
    
    // 상위 9개 항목에 대해 단축키 인덱스 [1] ~ [9] 를 터미널 폰트로 표시
    if (index >= 0 && index < 9) {
        std::wstring hotkey = L"[" + std::to_wstring(index + 1) + L"]";
        RECT hotkey_rect{text_left, face_rect.top, text_left + 24, face_rect.bottom};
        DrawTextBlock(dc, hotkey_rect, hotkey, detail_font, kAccentColor, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        text_left += 26;
    }

    const std::wstring label = ReadControlText(item->hwndItem);
    RECT title_rect{text_left, face_rect.top, face_rect.right - 10, face_rect.bottom};
    DrawTextBlock(dc, title_rect, label, title_font, text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    if (focused) {
        RECT focus_rect = InsetRectCopy(face_rect, 2, 2);
        DrawFocusRect(dc, &focus_rect);
    }
}

void DrawStyledButton(const DRAWITEMSTRUCT* item, HFONT font, HFONT detail_font) {
    DoubleBuffer buffer(item->hDC, item->rcItem);
    HDC dc = buffer.hdc();
    const int control_id = static_cast<int>(item->CtlID);
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    const bool focused = (item->itemState & ODS_FOCUS) != 0;
    const ButtonTone tone = ToneForButtonId(control_id);
    const LONG_PTR user_data = GetWindowLongPtrW(item->hwndItem, GWLP_USERDATA);
    const UiIcon icon = IconForButtonId(control_id, user_data);
    const bool icon_only = IconOnlyButton(control_id);
    const bool has_icon = HasButtonIcon(control_id);

    COLORREF fill = kPanelElevated;
    COLORREF border = kBorderColor;
    COLORREF text = kTextPrimary;
    COLORREF badge_fill = ScaleColor(kPanelBackground, 1.06);
    COLORREF badge_border = ScaleColor(kBorderColor, 1.06);

    switch (tone) {
    case ButtonTone::Accent:
        fill = pressed ? ScaleColor(kAccentColorSoft, 1.06) : kAccentColorSoft;
        border = pressed ? kAccentGlow : kAccentColor;
        badge_fill = pressed ? ScaleColor(kAccentColor, 0.94) : kAccentColor;
        badge_border = border;
        break;
    case ButtonTone::Danger:
        fill = pressed ? ScaleColor(kDangerColor, 0.66) : ScaleColor(kDangerColor, 0.52);
        border = pressed ? ScaleColor(kDangerColor, 1.2) : kDangerColor;
        badge_fill = pressed ? kDangerColorPressed : kDangerColor;
        badge_border = border;
        break;
    case ButtonTone::Tile:
        fill = pressed ? ScaleColor(kPanelElevated, 1.08) : kPanelElevated;
        border = focused || pressed ? kAccentColor : ScaleColor(kBorderColor, 1.04);
        badge_fill = ScaleColor(kPanelBackground, 1.04);
        badge_border = focused || pressed ? kAccentGlow : ScaleColor(kBorderColor, 1.06);
        break;
    case ButtonTone::Neutral:
        fill = pressed ? ScaleColor(kPanelBackground, 1.08) : kPanelBackground;
        border = pressed ? kAccentColor : kBorderColor;
        break;
    }

    if (disabled) {
        fill = ScaleColor(fill, 0.72);
        border = ScaleColor(border, 0.72);
        text = kTextMuted;
        badge_fill = ScaleColor(badge_fill, 0.72);
        badge_border = ScaleColor(badge_border, 0.72);
    }

    RECT face_rect = item->rcItem;
    if (pressed) {
        OffsetRect(&face_rect, 0, 1);
    }
    DrawRoundedRect(dc, face_rect, fill, border, 0);

    if (tone == ButtonTone::Tile) {
        const TileKind kind = static_cast<TileKind>(user_data);
        const RECT badge_rect{face_rect.left + 8, face_rect.top + 8, face_rect.left + 40, face_rect.bottom - 8};
        DrawIconBadge(dc, badge_rect, badge_fill, badge_border, icon);

        const std::wstring text_value = ReadControlText(item->hwndItem);
        const RECT title_rect{badge_rect.right + 12, face_rect.top + 8, face_rect.right - 24, face_rect.top + 24};
        const RECT subtitle_rect{badge_rect.right + 12, face_rect.top + 24, face_rect.right - 24, face_rect.bottom - 8};
        const RECT arrow_rect{face_rect.right - 24, face_rect.top + 16, face_rect.right - 8, face_rect.top + 32};

        DrawTextBlock(dc, title_rect, text_value, font, text, DT_LEFT | DT_TOP | DT_SINGLELINE);
        DrawTextBlock(dc, subtitle_rect, TileKindActionText(kind), detail_font != nullptr ? detail_font : font, kTextMuted, DT_LEFT | DT_TOP | DT_SINGLELINE);
        DrawUiIcon(dc, UiIcon::External, arrow_rect, pressed ? 255 : 200);
        return;
    }

    if (focused) {
        border = kAccentGlow;
        DrawRoundedRect(dc, face_rect, fill, border, 0);
    }

    if (icon_only) {
        RECT icon_rect = InsetRectCopy(face_rect, 6, 6);
        DrawUiIcon(dc, icon, icon_rect, disabled ? 170 : 255);
        return;
    }

    if (has_icon) {
        const RECT badge_rect{face_rect.left + 6, face_rect.top + 4, face_rect.left + 30, face_rect.bottom - 4};
        DrawIconBadge(dc, badge_rect, badge_fill, badge_border, icon);
        const RECT text_rect{badge_rect.right + 8, face_rect.top, face_rect.right - 10, face_rect.bottom};
        DrawTextBlock(dc, text_rect, ReadControlText(item->hwndItem), font, tone == ButtonTone::Danger ? kTextDanger : text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    DrawTextBlock(dc, face_rect, ReadControlText(item->hwndItem), font, text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawFavoriteListItem(const DRAWITEMSTRUCT* item, const FavoriteItem& favorite, HFONT title_font, HFONT detail_font) {
    DoubleBuffer buffer(item->hDC, item->rcItem);
    HDC dc = buffer.hdc();
    RECT outer = item->rcItem;
    FillRect(dc, &outer, PanelBrush());

    if (item->itemID == static_cast<UINT>(-1)) {
        return;
    }

    const bool selected = (item->itemState & ODS_SELECTED) != 0;
    const RECT face_rect{outer.left + 4, outer.top + 4, outer.right - 4, outer.bottom - 4};
    const COLORREF fill = selected ? ScaleColor(kPanelElevated, 1.06) : kPanelElevated;
    const COLORREF border = selected ? kAccentColor : kBorderColor;
    DrawRoundedRect(dc, face_rect, fill, border, 0);

    const RECT badge_rect{face_rect.left + 8, face_rect.top + 8, face_rect.left + 40, face_rect.bottom - 8};
    const TileKind kind = TileKindFromString(favorite.kind);
    
    if (kind == TileKind::Divider) {
        if (selected) {
            DrawRoundedRect(dc, face_rect, fill, border, 0);
        }
        RECT text_rect = face_rect;
        text_rect.left += 12;
        
        SIZE text_size{0, 0};
        if (!favorite.label.empty()) {
            const HGDIOBJ old_font = SelectObject(dc, detail_font);
            GetTextExtentPoint32W(dc, favorite.label.c_str(), static_cast<int>(favorite.label.length()), &text_size);
            SelectObject(dc, old_font);
            DrawTextBlock(dc, text_rect, favorite.label, detail_font, kAccentColor, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        
        RECT line_rect = face_rect;
        line_rect.left = text_rect.left + text_size.cx + (favorite.label.empty() ? 0 : 8);
        line_rect.right -= 12;
        line_rect.top = line_rect.top + (line_rect.bottom - line_rect.top) / 2;
        line_rect.bottom = line_rect.top + 1;
        DrawRoundedRect(dc, line_rect, ScaleColor(kBorderColor, 0.5), ScaleColor(kBorderColor, 0.5), 0);
        return;
    }

    DrawRoundedRect(dc, badge_rect, ScaleColor(kPanelBackground, 1.04), selected ? kAccentGlow : kBorderColor, 0);
    RECT badge_icon = InsetRectCopy(badge_rect, 6, 6);
    if (!DrawFavoriteTargetIcon(dc, favorite.kind, favorite.target, badge_icon)) {
        DrawUiIcon(dc, IconForTileKind(kind), badge_icon, 255);
    }

    const RECT title_rect{badge_rect.right + 12, face_rect.top + 8, face_rect.right - 12, face_rect.top + 24};
    const RECT subtitle_rect{badge_rect.right + 12, face_rect.top + 24, face_rect.right - 12, face_rect.bottom - 8};
    DrawTextBlock(dc, title_rect, favorite.label, title_font, kTextPrimary, DT_LEFT | DT_TOP | DT_SINGLELINE);
    DrawTextBlock(dc, subtitle_rect, KindDisplayName(favorite.kind) + L"  " + FileNameFromPath(favorite.target), detail_font != nullptr ? detail_font : title_font, kTextMuted, DT_LEFT | DT_TOP | DT_SINGLELINE);
}

void EnableModernWindowChrome(HWND hwnd) {
    BOOL dark_mode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_mode, sizeof(dark_mode));

    DWORD backdrop = DWMSBT_MAINWINDOW;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));

    DWORD corner_preference = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_preference, sizeof(corner_preference));

    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

std::wstring CountText(std::size_t count) {
    return std::to_wstring(count) + L"개";
}

std::wstring KindDisplayName(const std::wstring& kind) {
    if (kind == L"document") {
        return L"\uBB38\uC11C";
    }
    if (kind == L"url") {
        return L"\uB9C1\uD06C";
    }
    if (kind == L"divider") {
        return L"\uAD6C\uBD84\uC120";
    }
    return L"\uD504\uB85C\uADF8\uB7A8";
}

std::wstring NormalizeKind(const std::wstring& kind) {
    const std::wstring lowered = ToLower(Trim(kind));
    if (lowered == L"app" || lowered == L"document" || lowered == L"url" || lowered == L"divider") {
        return lowered;
    }
    return L"";
}

bool IsValidUrl(const std::wstring& value) {
    return IStartsWith(value, L"http://") || IStartsWith(value, L"https://");
}

std::wstring StripWrappingQuotes(const std::wstring& value) {
    if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
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

std::wstring FileStemFromPath(const std::wstring& path) {
    const std::wstring name = FileNameFromPath(path);
    const std::wstring::size_type dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos) {
        return name;
    }
    return name.substr(0, dot);
}

std::wstring DirectoryFromPath(const std::wstring& path) {
    const std::wstring::size_type slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, slash);
}

bool IsRegularFile(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring AbsolutePath(const std::wstring& path) {
    const DWORD required = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (required == 0) {
        return L"";
    }

    std::wstring buffer(static_cast<std::size_t>(required), L'\0');
    const DWORD written = GetFullPathNameW(path.c_str(), required, buffer.data(), nullptr);
    if (written == 0) {
        return L"";
    }

    buffer.resize(static_cast<std::size_t>(written));
    return buffer;
}

bool NormalizeFavoriteInput(
    const std::wstring& label,
    const std::wstring& kind,
    const std::wstring& target,
    FavoriteItem& normalized,
    std::wstring& error) {
    normalized.label = Trim(label);
    normalized.kind = NormalizeKind(kind);
    normalized.target = Trim(target);

    if (normalized.label.empty()) {
        error = L"\uB77C\uBCA8\uC744 \uC785\uB825\uD558\uC138\uC694.";
        return false;
    }

    if (normalized.kind.empty()) {
        error = L"\uC9C0\uC6D0\uD558\uC9C0 \uC54A\uB294 \uD56D\uBAA9 \uD0C0\uC785\uC785\uB2C8\uB2E4.";
        return false;
    }

    if (normalized.kind == L"divider") {
        normalized.target = L""; // 구분선은 경로가 필요 없음
        return true;
    }

    if (normalized.target.empty()) {
        error = L"\uB300\uC0C1\uC744 \uC785\uB825\uD558\uC138\uC694.";
        return false;
    }

    if (normalized.kind == L"url") {
        normalized.target = StripWrappingQuotes(normalized.target);
        if (!IsValidUrl(normalized.target)) {
            error = L"http/https \uB9C1\uD06C\uB9CC \uB4F1\uB85D\uD560 \uC218 \uC788\uC2B5\uB2C8\uB2E4.";
            return false;
        }
        return true;
    }

    normalized.target = StripWrappingQuotes(normalized.target);
    const std::wstring full_path = AbsolutePath(normalized.target);
    if (full_path.empty() || !IsRegularFile(full_path)) {
        error = L"\uB300\uC0C1 \uD30C\uC77C\uC744 \uCC3E\uC744 \uC218 \uC5C6\uC2B5\uB2C8\uB2E4.";
        return false;
    }

    normalized.target = full_path;
    if (normalized.kind == L"app" && FileExtensionFromPath(full_path) != L".exe") {
        error = L"\uD504\uB85C\uADF8\uB7A8 \uD56D\uBAA9\uC740 .exe \uD30C\uC77C\uB9CC \uB4F1\uB85D\uD560 \uC218 \uC788\uC2B5\uB2C8\uB2E4.";
        return false;
    }

    return true;
}

bool LaunchTarget(const FavoriteItem& favorite, std::wstring& error) {
    if (favorite.kind == L"url" || favorite.kind == L"document") {
        const INT_PTR result = reinterpret_cast<INT_PTR>(
            ShellExecuteW(nullptr, L"open", favorite.target.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
        if (result <= 32) {
            error = favorite.kind == L"url"
                ? L"\uB9C1\uD06C\uB97C \uC5F4\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4."
                : L"\uBB38\uC11C\uB97C \uC5F4\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.";
            return false;
        }
        return true;
    }

    std::wstring command = L"\"" + favorite.target + L"\"";
    const std::wstring working_directory = DirectoryFromPath(favorite.target);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};

    if (!CreateProcessW(
            favorite.target.c_str(),
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            working_directory.empty() ? nullptr : working_directory.c_str(),
            &startup,
            &process)) {
        error = L"\uD504\uB85C\uADF8\uB7A8\uC744 \uC2E4\uD589\uD558\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.";
        return false;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

void KeepWidgetOnTop(HWND hwnd) {
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

LRESULT CALLBACK SettingsListBoxSubclassProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param, UINT_PTR id_subclass, DWORD_PTR ref_data) {
    static bool is_dragging = false;
    static int drag_index = -1;

    switch (message) {
    case WM_LBUTTONDOWN: {
        LRESULT result = DefSubclassProc(hwnd, message, w_param, l_param);
        DWORD pos = SendMessageW(hwnd, LB_ITEMFROMPOINT, 0, l_param);
        if (HIWORD(pos) == 0) { // 리스트박스 내부의 유효한 항목인지 확인
            is_dragging = true;
            drag_index = LOWORD(pos);
            SetCapture(hwnd); // 마우스가 창 밖으로 나가도 이벤트를 받기 위해 캡처
        }
        return result;
    }
    case WM_MOUSEMOVE: {
        if (is_dragging) {
            SetCursor(LoadCursorW(nullptr, IDC_SIZENS)); // 상하 화살표 커서로 변경
            return 0; // 드래그 중에 항목이 파랗게 선택되는 기본 동작을 막음
        }
        break;
    }
    case WM_LBUTTONUP: {
        if (is_dragging) {
            is_dragging = false;
            ReleaseCapture();
            DWORD pos = SendMessageW(hwnd, LB_ITEMFROMPOINT, 0, l_param);
            if (HIWORD(pos) == 0) {
                int drop_index = LOWORD(pos);
                if (drag_index != -1 && drop_index != -1 && drag_index != drop_index) {
                    HWND parent = GetParent(hwnd);
                    SendMessageW(parent, kMessageMoveFavorite, drag_index, drop_index);
                }
            }
            drag_index = -1;
            return 0; // 놓았을 때 해당 위치로 바로 선택이 튀는 것을 방지
        }
        break;
    }
    case WM_SETCURSOR: {
        if (is_dragging) {
            SetCursor(LoadCursorW(nullptr, IDC_SIZENS));
            return TRUE;
        }
        break;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, SettingsListBoxSubclassProc, id_subclass);
        break;
    }
    return DefSubclassProc(hwnd, message, w_param, l_param);
}
}  // namespace

bool WindowBase::Register(HINSTANCE instance) const {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = ClassStyle();
    window_class.lpfnWndProc = StaticWindowProc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = BackgroundBrush();
    window_class.lpszClassName = ClassName();

    return RegisterClassExW(&window_class) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

LRESULT CALLBACK WindowBase::StaticWindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param) {
    WindowBase* self = nullptr;

    if (message == WM_NCCREATE) {
        const auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(l_param);
        self = static_cast<WindowBase*>(create_struct->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<WindowBase*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->HandleMessage(message, w_param, l_param);
    }

    return DefWindowProcW(hwnd, message, w_param, l_param);
}

MainWidgetWindow::MainWidgetWindow(FavoriteApp* app) : app_(app) {}

MainWidgetWindow::~MainWidgetWindow() {
    if (detail_font_ != nullptr) {
        DeleteObject(detail_font_);
    }
    if (ui_font_ != nullptr) {
        DeleteObject(ui_font_);
    }
    if (title_font_ != nullptr) {
        DeleteObject(title_font_);
    }
}

bool MainWidgetWindow::Create(HINSTANCE instance) {
    if (!Register(instance)) {
        return false;
    }

    const auto& widget = app_->state().widget;
    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        ClassName(),
        kTitleText,
        WS_POPUP | WS_CLIPCHILDREN | WS_VSCROLL,
        widget.x,
        widget.y,
        260,
        420,
        nullptr,
        nullptr,
        instance,
        this);

    if (hwnd_ != nullptr) {
        EnableModernWindowChrome(hwnd_);
        // Ctrl + Shift + F 를 전역 단축키로 등록 (0x4000 = MOD_NOREPEAT, 키 꾹 누름 방지)
        RegisterHotKey(hwnd_, kIdGlobalHotkey, MOD_CONTROL | MOD_SHIFT | 0x4000, 'F');
        
        // 상위 9개 항목에 대해 Ctrl + Shift + 1~9 전역 단축키 등록
        for (int i = 0; i < 9; ++i) {
            RegisterHotKey(hwnd_, kIdGlobalHotkey + 1 + i, MOD_CONTROL | MOD_SHIFT | 0x4000, '1' + i);
        }
    }

    return hwnd_ != nullptr;
}

void MainWidgetWindow::Show() {
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    SetWindowPos(
        hwnd_,
        HWND_TOPMOST,
        app_->state().widget.x,
        app_->state().widget.y,
        260,
        420,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
    KeepWidgetOnTop(hwnd_);
}

void MainWidgetWindow::Refresh() {
    SetWindowTextW(count_label_, CountText(app_->state().favorites.size()).c_str());
    RebuildButtons();
    UpdateScrollBar();
    LayoutControls();
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void MainWidgetWindow::ShowError(const std::wstring& message) {
    SetWindowTextW(status_label_, message.c_str());
    ShowWindow(status_label_, message.empty() ? SW_HIDE : SW_SHOW);
    UpdateScrollBar();
    LayoutControls();
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

const wchar_t* MainWidgetWindow::ClassName() const {
    return L"FavoriteWidgetMainWindow";
}

HBRUSH MainWidgetWindow::BackgroundBrush() const {
    return WidgetBrush();
}

LRESULT MainWidgetWindow::HandleMessage(UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
    case WM_CREATE:
        CreateFonts();
        CreateControls();
        Refresh();
        return 0;

    case WM_SIZE:
        UpdateScrollBar();
        LayoutControls();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_MOVE:
        if (!IsIconic(hwnd_)) {
            app_->UpdateWidgetPosition(
                static_cast<int>(static_cast<short>(LOWORD(l_param))),
                static_cast<int>(static_cast<short>(HIWORD(l_param))));
        }
        return 0;

    case WM_MOUSEWHEEL:
        ScrollTo(target_scroll_offset_ - (GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA) * 20);
        return 0;

    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_HOTKEY:
        if (w_param == kIdGlobalHotkey) {
            if (IsWindowVisible(hwnd_)) {
                ShowWindow(hwnd_, SW_HIDE);
            } else {
                ShowWindow(hwnd_, SW_SHOW);
                SetForegroundWindow(hwnd_);
            }
            return 0;
        }
        // Ctrl + Shift + 1~9 다이렉트 실행 처리
        else if (w_param >= kIdGlobalHotkey + 1 && w_param <= kIdGlobalHotkey + 9) {
            const std::size_t index = w_param - (kIdGlobalHotkey + 1);
            std::wstring error;
            if (index < app_->state().favorites.size()) {
                if (app_->state().favorites[index].kind == L"divider") {
                    return 0; // 구분선은 단축키 실행 안 함
                }
                if (!app_->LaunchFavorite(index, error)) {
                    ShowError(error);
                    ShowWindow(hwnd_, SW_SHOW);
                    SetForegroundWindow(hwnd_);
                } else {
                    ShowWindow(hwnd_, SW_HIDE); // 실행 성공 시 위젯 숨김
                }
            }
            return 0;
        }
        break;

    case WM_VSCROLL: {
        SCROLLINFO info{};
        info.cbSize = sizeof(info);
        info.fMask = SIF_ALL;
        GetScrollInfo(hwnd_, SB_VERT, &info);

        int next = target_scroll_offset_;
        switch (LOWORD(w_param)) {
        case SB_LINEUP:
            next -= 20;
            break;
        case SB_LINEDOWN:
            next += 20;
            break;
        case SB_PAGEUP:
            next -= static_cast<int>(info.nPage);
            break;
        case SB_PAGEDOWN:
            next += static_cast<int>(info.nPage);
            break;
        case SB_THUMBTRACK:
            next = info.nTrackPos;
            break;
        default:
            break;
        }

        ScrollTo(next);
        return 0;
    }

    case WM_COMMAND: {
        const WORD control_id = LOWORD(w_param);
        if (control_id == kIdSettingsButton) {
            app_->OpenSettingsWindow();
            KeepWidgetOnTop(hwnd_);
            return 0;
        }
        if (control_id == kIdCloseButton) {
            ShowWindow(hwnd_, SW_HIDE); // 앱을 종료하지 않고 위젯만 숨김 처리
            return 0;
        }

        if (control_id >= kIdFavoriteButtonBase) {
            const auto index = HitFavoriteIndex(control_id);
            if (index.has_value()) {
                if (app_->state().favorites[*index].kind == L"divider") {
                    return 0; // 구분선은 클릭 무시
                }
                std::wstring error;
                if (!app_->LaunchFavorite(*index, error)) {
                    ShowError(error);
                    KeepWidgetOnTop(hwnd_);
                } else {
                    ShowError(L"");
                    ShowWindow(hwnd_, SW_HIDE); // 마우스 클릭으로 실행 성공 시에도 즉시 위젯 숨김
                }
            }
            return 0;
        }
        if (control_id == kIdButtonBackup) {
            wchar_t file_path[MAX_PATH] = L"favorit_backup.ini";
            OPENFILENAMEW ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd_;
            ofn.lpstrFilter = L"\uC124\uC815 \uD30C\uC77C (*.ini)\0*.ini\0\uBAA8\uB4E0 \uD30C\uC77C (*.*)\0*.*\0";
            ofn.lpstrFile = file_path;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt = L"ini";

            if (GetSaveFileNameW(&ofn)) {
                // TODO: 사용자 백업 로직 구현 (Storage::Save 등 연동)
                UpdateStatus(L"\uBC31\uC5C5 \uB300\uAE30 \uACBD\uB85C: " + std::wstring(file_path));
            }
            return 0;
        }
        if (control_id == kIdButtonRestore) {
            wchar_t file_path[MAX_PATH] = L"";
            OPENFILENAMEW ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd_;
            ofn.lpstrFilter = L"\uC124\uC815 \uD30C\uC77C (*.ini)\0*.ini\0\uBAA8\uB4E0 \uD30C\uC77C (*.*)\0*.*\0";
            ofn.lpstrFile = file_path;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt = L"ini";

            if (GetOpenFileNameW(&ofn)) {
                // TODO: 사용자 복원 로직 구현 (Storage::Load 연동 및 RefreshAllWindows 호출)
                UpdateStatus(L"\uBCF5\uC6D0 \uB300\uAE30 \uACBD\uB85C: " + std::wstring(file_path));
            }
            return 0;
        }
        return 0;
    }

    case WM_CONTEXTMENU: {
        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        if (point.x == -1 && point.y == -1) {
            GetCursorPos(&point);
        }

        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, kIdSettingsButton, L"\uC124\uC815");
        AppendMenuW(menu, MF_STRING, kIdExitMenu, L"\uC885\uB8CC");

        const int selected = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, hwnd_, nullptr);
        DestroyMenu(menu);

        if (selected == kIdSettingsButton) {
            app_->OpenSettingsWindow();
        } else if (selected == kIdExitMenu) {
            DestroyWindow(hwnd_);
        }
        return 0;
    }

    case WM_TIMER:
        if (w_param == 2) { // 부드러운 스크롤 애니메이션 타이머
            if (scroll_offset_ != target_scroll_offset_) {
                int diff = target_scroll_offset_ - scroll_offset_;
                int step = diff / 4; // 값이 작을수록 느리게, 클수록 빠르게(감속 효과)
                if (step == 0) {
                    step = diff > 0 ? 1 : -1;
                }
                scroll_offset_ += step;

                SCROLLINFO info{};
                info.cbSize = sizeof(info);
                info.fMask = SIF_POS;
                info.nPos = scroll_offset_;
                SetScrollInfo(hwnd_, SB_VERT, &info, TRUE);

                LayoutControls();
            } else {
                KillTimer(hwnd_, 2);
                scroll_timer_id_ = 0;
            }
            return 0;
        }
        DestroyWindow(hwnd_);
        return 0;

    case WM_NCHITTEST: {
        const LRESULT hit = DefWindowProcW(hwnd_, message, w_param, l_param);
        if (hit != HTCLIENT) {
            return hit;
        }

        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ScreenToClient(hwnd_, &point);
        const HWND child = ChildWindowFromPointEx(hwnd_, point, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
        if (child == nullptr || child == hwnd_ || child == title_label_ || child == subtitle_label_ || child == count_label_ || child == status_label_) {
            return HTCAPTION;
        }
        return HTCLIENT;
    }

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd_, &paint);
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

        RECT header = rect;
        header.left += 8;
        header.top += 8;
        header.right -= 8;
        header.bottom = header.top + 42;
        DrawRoundedRect(dc, header, ScaleColor(kPanelBackground, 1.01), kBorderColor, 0);

        RECT grid_panel = rect;
        grid_panel.left += 8;
        grid_panel.right -= 8;
        grid_panel.top = header.bottom + 8;
        grid_panel.bottom -= 8;
        DrawRoundedRect(dc, grid_panel, ScaleColor(kPanelBackground, 0.98), kBorderColor, 0);

        RECT title_rule{header.left + 10, header.top + 10, header.left + 30, header.top + 12};
        DrawRoundedRect(dc, title_rule, kAccentColor, kAccentColor, 0);

        if (favorite_buttons_.empty()) {
            const RECT empty_badge{
                grid_panel.left + ((grid_panel.right - grid_panel.left) - 48) / 2,
                grid_panel.top + 30,
                grid_panel.left + ((grid_panel.right - grid_panel.left) + 48) / 2,
                grid_panel.top + 78};
            DrawIconBadge(dc, empty_badge, ScaleColor(kAccentColorSoft, 1.05), kBorderColor, UiIcon::Star);

            RECT title_rect{grid_panel.left + 16, empty_badge.bottom + 16, grid_panel.right - 16, empty_badge.bottom + 36};
            RECT body_rect{grid_panel.left + 24, empty_badge.bottom + 40, grid_panel.right - 24, empty_badge.bottom + 80};
            DrawTextBlock(dc, title_rect, kEmptyStateTitle, ui_font_, kTextPrimary, DT_CENTER | DT_TOP | DT_SINGLELINE);
            DrawTextBlock(dc, body_rect, kEmptyStateSubtitle, detail_font_ != nullptr ? detail_font_ : ui_font_, kTextMuted, DT_CENTER | DT_TOP | DT_WORDBREAK);
        }

        EndPaint(hwnd_, &paint);
        return 0;
    }

    case WM_DRAWITEM:
        if (l_param != 0) {
            const auto* item = reinterpret_cast<const DRAWITEMSTRUCT*>(l_param);
            if (item->CtlID >= kIdFavoriteButtonBase) {
                DrawFavoriteListButton(item, ui_font_, detail_font_);
            } else {
                DrawStyledButton(item, ui_font_, detail_font_);
            }
        }
        return TRUE;

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(w_param);
        const HWND control = reinterpret_cast<HWND>(l_param);
        COLORREF color = kWidgetText;
        if (control == subtitle_label_) {
            color = kTextMuted;
        } else if (control == count_label_) {
            color = kTextPrimary;
        } else if (control == status_label_) {
            color = kTextDanger;
        }
        SetTextColor(dc, color);
        SetBkMode(dc, TRANSPARENT);
        
        if (control == status_label_) {
            return reinterpret_cast<LRESULT>(GridBrush());
        }
        return reinterpret_cast<LRESULT>(HeaderBrush());
    }

    case WM_DESTROY:
        for (int i = 0; i < 10; ++i) {
            UnregisterHotKey(hwnd_, kIdGlobalHotkey + i);
        }
        if (app_->settings_window() != nullptr && IsWindow(app_->settings_window()->hwnd())) {
            DestroyWindow(app_->settings_window()->hwnd());
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd_, message, w_param, l_param);
}

void MainWidgetWindow::CreateFonts() {
    ui_font_ = CreateUiFont(11, FW_SEMIBOLD);
    title_font_ = CreateUiFont(14, FW_BOLD, L"Segoe UI Variable Display");
    detail_font_ = CreateUiFont(10, FW_NORMAL, L"Consolas");
}

void MainWidgetWindow::CreateControls() {
    const HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd_, GWLP_HINSTANCE));

    title_label_ = CreateWindowExW(0, L"STATIC", kTitleText, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    subtitle_label_ = CreateWindowExW(0, L"STATIC", kWidgetSubtitleText, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    count_label_ = CreateWindowExW(0, L"STATIC", CountText(0).c_str(), WS_CHILD | WS_VISIBLE | SS_RIGHT, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    settings_button_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdSettingsButton),
        instance,
        nullptr);
    close_button_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"X",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdCloseButton),
        instance,
        nullptr);
    status_label_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);

    ApplyFont(title_label_, title_font_);
    ApplyFont(subtitle_label_, detail_font_);
    ApplyFont(count_label_, ui_font_);
    ApplyFont(settings_button_, ui_font_);
    ApplyFont(close_button_, ui_font_);
    ApplyFont(status_label_, detail_font_);
}

void MainWidgetWindow::DestroyDynamicButtons() {
    for (HWND button : favorite_buttons_) {
        ForgetFavoriteVisual(button);
        DestroyWindow(button);
    }
    favorite_buttons_.clear();
}

void MainWidgetWindow::LayoutControls() {
    RECT client{};
    GetClientRect(hwnd_, &client);

    const int margin = 8;
    const int hero_height = 42;
    const int action_size = 24;
    const int count_width = 46;
    const int gap = 6;
    const int grid_padding = 8;
    const bool show_status = IsWindowVisible(status_label_) != FALSE;
    const int status_height = show_status ? 18 : 0;
    const int columns = 1;

    const RECT hero{margin, margin, client.right - margin, margin + hero_height};
    const RECT shelf{margin, hero.bottom + 8, client.right - margin, client.bottom - margin};
    const int title_left = hero.left + 10;
    const int close_x = hero.right - 8 - action_size;
    const int settings_x = close_x - 6 - action_size;
    const int count_x = settings_x - 8 - count_width;
    const int text_width = count_x - title_left - 8;
    const int shelf_width = shelf.right - shelf.left - grid_padding * 2;
    const int button_width = shelf_width;
    const int button_height = 36;

    const int num_windows = 5 + (show_status ? 1 : 0) + static_cast<int>(favorite_buttons_.size());
    HDWP hdwp = BeginDeferWindowPos(num_windows);

    if (hdwp) hdwp = DeferWindowPos(hdwp, title_label_, nullptr, title_left, hero.top + 6, text_width, 18, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hdwp) hdwp = DeferWindowPos(hdwp, subtitle_label_, nullptr, title_left, hero.top + 24, text_width, 14, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hdwp) hdwp = DeferWindowPos(hdwp, count_label_, nullptr, count_x, hero.top + 12, count_width, 18, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hdwp) hdwp = DeferWindowPos(hdwp, settings_button_, nullptr, settings_x, hero.top + 9, action_size, action_size, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hdwp) hdwp = DeferWindowPos(hdwp, close_button_, nullptr, close_x, hero.top + 9, action_size, action_size, SWP_NOZORDER | SWP_NOACTIVATE);

    if (show_status) {
        if (hdwp) hdwp = DeferWindowPos(hdwp, status_label_, nullptr, shelf.left + 18, shelf.bottom - 16 - status_height, shelf.right - shelf.left - 36, status_height, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    for (std::size_t index = 0; index < favorite_buttons_.size(); ++index) {
        const int row = static_cast<int>(index);
        const int column = 0;
        const int row_start = row * columns;
        const int row_count = std::min<int>(columns, static_cast<int>(favorite_buttons_.size()) - row_start);
        const int row_width = row_count * button_width + (row_count - 1) * gap;
        const int row_left = shelf.left + grid_padding + std::max((shelf_width - row_width) / 2, 0);
        const int x = row_left + column * (button_width + gap);
        const int y = shelf.top + 12 + row * (button_height + gap) - scroll_offset_;
        if (hdwp) hdwp = DeferWindowPos(hdwp, favorite_buttons_[index], nullptr, x, y, button_width, button_height, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (hdwp) {
        EndDeferWindowPos(hdwp);
    }

    HRGN region = CreateRectRgn(0, 0, client.right, client.bottom);
    SetWindowRgn(hwnd_, region, TRUE);
}

void MainWidgetWindow::RebuildButtons() {
    DestroyDynamicButtons();

    const HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd_, GWLP_HINSTANCE));
    const auto& favorites = app_->state().favorites;
    for (std::size_t index = 0; index < favorites.size(); ++index) {
        HWND button = CreateWindowExW(
            0,
            L"BUTTON",
            favorites[index].label.c_str(),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | (favorites[index].kind == L"divider" ? WS_DISABLED : 0),
            0,
            0,
            0,
            0,
            hwnd_,
            reinterpret_cast<HMENU>(kIdFavoriteButtonBase + static_cast<int>(index)),
            instance,
            nullptr);
        ApplyFont(button, ui_font_);
        RememberFavoriteVisual(button, favorites[index]);
        favorite_buttons_.push_back(button);
    }
}

void MainWidgetWindow::UpdateScrollBar() {
    RECT client{};
    GetClientRect(hwnd_, &client);

    const int margin = 8;
    const int hero_height = 42;
    const int gap = 6;
    const int columns = 1;
    const bool show_status = IsWindowVisible(status_label_) != FALSE;
    const int status_height = show_status ? 18 + 18 : 0;
    const int shelf_height = client.bottom - (margin + hero_height + 8) - margin;
    const int button_height = 36;
    const int visible_height = std::max(shelf_height - 36 - status_height, 0);
    const int rows = static_cast<int>(favorite_buttons_.size());
    const int content_height = rows == 0 ? 0 : rows * button_height + (rows - 1) * gap;

    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMin = 0;
    info.nMax = std::max(content_height - 1, 0);
    info.nPage = static_cast<UINT>(visible_height);
    
    const int max_offset = std::max(content_height - visible_height, 0);
    target_scroll_offset_ = std::min(target_scroll_offset_, max_offset);
    scroll_offset_ = std::min(scroll_offset_, max_offset);
    
    info.nPos = scroll_offset_;
    SetScrollInfo(hwnd_, SB_VERT, &info, TRUE);
    ShowScrollBar(hwnd_, SB_VERT, content_height > visible_height);

    if (scroll_offset_ != target_scroll_offset_ && scroll_timer_id_ == 0) {
        scroll_timer_id_ = SetTimer(hwnd_, 2, 16, nullptr);
    }
}

void MainWidgetWindow::ScrollTo(int offset) {
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    GetScrollInfo(hwnd_, SB_VERT, &info);

    const int max_offset = std::max(info.nMax - static_cast<int>(info.nPage) + 1, 0);
    target_scroll_offset_ = std::clamp(offset, 0, max_offset);
    
    if (scroll_offset_ != target_scroll_offset_ && scroll_timer_id_ == 0) {
        scroll_timer_id_ = SetTimer(hwnd_, 2, 16, nullptr); // 16ms (약 60FPS) 타이머 시작
    }
}

std::optional<std::size_t> MainWidgetWindow::HitFavoriteIndex(WORD control_id) const {
    const int index = static_cast<int>(control_id) - kIdFavoriteButtonBase;
    if (index < 0 || static_cast<std::size_t>(index) >= app_->state().favorites.size()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(index);
}

SettingsWindow::SettingsWindow(FavoriteApp* app) : app_(app) {}

SettingsWindow::~SettingsWindow() {
    CloseSearchDialog();
    if (detail_font_ != nullptr) {
        DeleteObject(detail_font_);
    }
    if (ui_font_ != nullptr) {
        DeleteObject(ui_font_);
    }
}

bool SettingsWindow::Create(HINSTANCE instance, HWND owner) {
    owner_ = owner;
    if (!Register(instance)) {
        return false;
    }

    hwnd_ = CreateWindowExW(
        WS_EX_APPWINDOW,
        ClassName(),
        kSettingsTitleText,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        kSettingsWidth,
        kSettingsHeight,
        owner,
        nullptr,
        instance,
        this);

    if (hwnd_ != nullptr) {
        EnableModernWindowChrome(hwnd_);
    }

    return hwnd_ != nullptr;
}

void SettingsWindow::Show() {
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd_);
}

void SettingsWindow::Refresh() {
    UpdateMetaText();
    ReloadList();
    LayoutControls();
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void SettingsWindow::SetSearchResult(const std::wstring& path) {
    if (path.empty()) {
        return;
    }

    SetWindowTextW(target_edit_, path.c_str());
    if (Trim(ReadWindowText(label_edit_)).empty()) {
        SetWindowTextW(label_edit_, FileStemFromPath(path).c_str());
    }
    UpdateStatus(L"");
}

void SettingsWindow::OnSearchDialogClosed() {
    search_dialog_ = nullptr;
}

const wchar_t* SettingsWindow::ClassName() const {
    return L"FavoriteWidgetSettingsWindow";
}

HBRUSH SettingsWindow::BackgroundBrush() const {
    return WindowBrush();
}

LRESULT SettingsWindow::HandleMessage(UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
    case WM_CREATE:
        CreateFonts();
        CreateControls();
        LayoutControls();
        Refresh();
        return 0;

    case WM_SIZE:
        LayoutControls();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd_, &paint);
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

        RECT hero = rect;
        hero.left += 12;
        hero.top += 12;
        hero.right -= 12;
        hero.bottom = hero.top + 54;
        DrawRoundedRect(dc, hero, ScaleColor(kPanelBackground, 1.01), kBorderColor, 0);

        const int content_top = hero.bottom + 10;
        const int content_bottom = rect.bottom - 12;
        const int content_width = hero.right - hero.left;
        const int left_width = std::clamp(content_width / 3, 260, 320);
        RECT left_panel{hero.left, content_top, hero.left + left_width, content_bottom};
        RECT right_panel{left_panel.right + 14, content_top, hero.right, content_bottom};
        DrawRoundedRect(dc, left_panel, ScaleColor(kPanelBackground, 0.99), kBorderColor, 0);
        DrawRoundedRect(dc, right_panel, ScaleColor(kPanelBackground, 0.98), kBorderColor, 0);

        RECT accent_strip{hero.left + 12, hero.top + 10, hero.left + 64, hero.top + 12};
        DrawRoundedRect(dc, accent_strip, kAccentColor, kAccentColor, 0);

        EndPaint(hwnd_, &paint);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(w_param);
        HWND control = reinterpret_cast<HWND>(l_param);
        COLORREF color = kTextPrimary;
        if (control == meta_label_ || control == favorites_label_ || control == name_label_ || control == kind_label_ || control == target_label_) {
            color = kTextMuted;
        } else if (control == status_label_) {
            color = kAccentGlow;
        }
        SetTextColor(dc, color);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(WindowBrush());
    }

    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(w_param);
        SetTextColor(dc, kTextPrimary);
        SetBkColor(dc, kInputBackground);
        return reinterpret_cast<LRESULT>(InputBrush());
    }

    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(w_param);
        const HWND control = reinterpret_cast<HWND>(l_param);
        SetTextColor(dc, kTextPrimary);
        if (control == list_box_) {
            SetBkColor(dc, kPanelElevated);
            return reinterpret_cast<LRESULT>(ElevatedBrush());
        }
        SetBkColor(dc, kInputBackground);
        return reinterpret_cast<LRESULT>(InputBrush());
    }

    case WM_DRAWITEM:
        if (l_param != 0) {
            const auto* item = reinterpret_cast<const DRAWITEMSTRUCT*>(l_param);
            if (item->CtlID == kIdListFavorites) {
                if (item->itemID < app_->state().favorites.size()) {
                    DrawFavoriteListItem(item, app_->state().favorites[item->itemID], ui_font_, detail_font_);
                }
            } else {
                DrawStyledButton(item, ui_font_, detail_font_);
            }
        }
        return TRUE;

    case WM_MEASUREITEM:
        if (w_param == kIdListFavorites) {
            auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(l_param);
            measure->itemHeight = 54;
            return TRUE;
        }
        break;

    case WM_COMMAND: {
        const WORD control_id = LOWORD(w_param);
        const WORD notify_code = HIWORD(w_param);

        if (control_id == kIdComboKind && notify_code == CBN_SELCHANGE) {
            const bool is_divider = CurrentKind() == L"divider";
            EnableWindow(target_edit_, !is_divider);
            EnableWindow(search_button_, !is_divider);
            return 0;
        }

        if (control_id == kIdListFavorites && notify_code == LBN_SELCHANGE) {
            LoadSelectionIntoForm();
            return 0;
        }
        if (control_id == kIdButtonSearch) {
            OpenSearchDialog();
            return 0;
        }
        if (control_id == kIdButtonSave) {
            HandleSave();
            return 0;
        }
        if (control_id == kIdButtonReset) {
            ResetForm();
            UpdateStatus(L"");
            return 0;
        }
        if (control_id == kIdButtonDelete) {
            HandleDelete();
            return 0;
        }
        if (control_id == kIdButtonMoveUp || control_id == kIdButtonMoveDown) {
            const int sel_idx = static_cast<int>(SendMessageW(list_box_, LB_GETCURSEL, 0, 0));
            if (sel_idx == LB_ERR) {
                UpdateStatus(L"\uC21C\uC11C\uB97C \uBCC0\uACBD\uD560 \uD56D\uBAA9\uC744 \uC120\uD0DD\uD558\uC138\uC694."); // 순서를 변경할 항목을 선택하세요.
                return 0;
            }
            
            int target_idx = (control_id == kIdButtonMoveUp) ? sel_idx - 1 : sel_idx + 1;
            auto& favs = const_cast<std::vector<FavoriteItem>&>(app_->state().favorites);
            
            if (target_idx >= 0 && target_idx < static_cast<int>(favs.size())) {
                std::swap(favs[sel_idx], favs[target_idx]);
                std::wstring error;
                if (app_->Persist(&error)) {
                    app_->RefreshAllWindows();
                } else {
                    std::swap(favs[sel_idx], favs[target_idx]); // 되돌리기
                    UpdateStatus(error);
                }
            }
            return 0;
        }
        return 0;
    }

    case kMessageMoveFavorite: {
        int from_index = static_cast<int>(w_param);
        int to_index = static_cast<int>(l_param);
        auto& favs = const_cast<std::vector<FavoriteItem>&>(app_->state().favorites);
        
        if (from_index >= 0 && from_index < static_cast<int>(favs.size()) && 
            to_index >= 0 && to_index < static_cast<int>(favs.size())) {
            
            FavoriteItem item = favs[from_index];
            favs.erase(favs.begin() + from_index);
            favs.insert(favs.begin() + to_index, item);
            
            std::wstring error;
            if (app_->Persist(&error)) {
                app_->RefreshAllWindows();
                SendMessageW(list_box_, LB_SETCURSEL, to_index, 0);
                LoadSelectionIntoForm();
            } else {
                UpdateStatus(error);
            }
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd_);
        return 0;

    case WM_DESTROY:
        CloseSearchDialog();
        return 0;

    case WM_NCDESTROY:
        app_->OnSettingsClosed();
        delete this;
        return 0;
    }

    return DefWindowProcW(hwnd_, message, w_param, l_param);
}

void SettingsWindow::CreateFonts() {
    ui_font_ = CreateUiFont(12, FW_SEMIBOLD);
    detail_font_ = CreateUiFont(11, FW_NORMAL, L"Consolas");
}

void SettingsWindow::CreateControls() {
    const HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd_, GWLP_HINSTANCE));

    meta_label_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    status_label_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    favorites_label_ = CreateWindowExW(0, L"STATIC", L"\uB4F1\uB85D\uB41C \uD56D\uBAA9", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    list_box_ = CreateWindowExW(
        0,
        L"LISTBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdListFavorites),
        instance,
        nullptr);
    name_label_ = CreateWindowExW(0, L"STATIC", L"\uD45C\uC2DC \uC774\uB984", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    label_edit_ = CreateWindowExW(
        0,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdEditLabel),
        instance,
        nullptr);
    kind_label_ = CreateWindowExW(0, L"STATIC", L"\uC2E4\uD589 \uBC29\uC2DD", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    kind_combo_ = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdComboKind),
        instance,
        nullptr);
    target_label_ = CreateWindowExW(0, L"STATIC", L"\uC2E4\uD589 \uB300\uC0C1", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    target_edit_ = CreateWindowExW(
        0,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdEditTarget),
        instance,
        nullptr);
    search_button_ = CreateWindowExW(0, L"BUTTON", L"\uD30C\uC77C \uAC80\uC0C9", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonSearch), instance, nullptr);
    save_button_ = CreateWindowExW(0, L"BUTTON", L"\uC800\uC7A5", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonSave), instance, nullptr);
    reset_button_ = CreateWindowExW(0, L"BUTTON", L"\uC0C8 \uD56D\uBAA9", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonReset), instance, nullptr);
    delete_button_ = CreateWindowExW(0, L"BUTTON", L"\uC81C\uAC70", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonDelete), instance, nullptr);
    HWND move_up_btn = CreateWindowExW(0, L"BUTTON", L"\u25B2 \uC704\uB85C", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonMoveUp), instance, nullptr);
    HWND move_down_btn = CreateWindowExW(0, L"BUTTON", L"\u25BC \uC544\uB798\uB85C", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonMoveDown), instance, nullptr);
    HWND backup_btn = CreateWindowExW(0, L"BUTTON", L"\uBC31\uC5C5", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonBackup), instance, nullptr);
    HWND restore_btn = CreateWindowExW(0, L"BUTTON", L"\uBCF5\uC6D0", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdButtonRestore), instance, nullptr);

    SendMessageW(kind_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\uD504\uB85C\uADF8\uB7A8(.exe)"));
    SendMessageW(kind_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\uBB38\uC11C/\uD30C\uC77C"));
    SendMessageW(kind_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\uB9C1\uD06C(URL)"));
    SendMessageW(kind_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"\uAD6C\uBD84\uC120(\uADF8\uB8F9)"));
    SendMessageW(kind_combo_, CB_SETCURSEL, 0, 0);
    SendMessageW(list_box_, LB_SETITEMHEIGHT, 0, 54);
    SendMessageW(kind_combo_, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 26);
    SendMessageW(kind_combo_, CB_SETITEMHEIGHT, 0, 24);
    SendMessageW(label_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(10, 10));
    SendMessageW(target_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(10, 10));

    ApplyFont(meta_label_, detail_font_);
    ApplyFont(status_label_, detail_font_);
    for (HWND control : {favorites_label_, name_label_, kind_label_, target_label_}) {
        ApplyFont(control, detail_font_);
    }
    for (HWND control : {list_box_, label_edit_, kind_combo_, target_edit_, search_button_, save_button_, reset_button_, delete_button_, move_up_btn, move_down_btn, backup_btn, restore_btn}) {
        ApplyFont(control, ui_font_);
    }

    SendMessageW(label_edit_, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"\uD45C\uC2DC \uB77C\uBCA8"));
    SendMessageW(target_edit_, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"\uACBD\uB85C \uB610\uB294 URL"));

    SetWindowSubclass(list_box_, SettingsListBoxSubclassProc, 1, 0);
}

void SettingsWindow::LayoutControls() {
    RECT client{};
    GetClientRect(hwnd_, &client);

    const int margin = 12;
    const bool show_status = IsWindowVisible(status_label_) != FALSE;
    const int hero_height = 54;
    const int gap = 10;
    const RECT hero{margin, 12, client.right - margin, 12 + hero_height};
    const int content_top = hero.bottom + gap;
    const int content_bottom = client.bottom - margin;
    const int content_width = hero.right - hero.left;
    const int left_width = std::clamp(content_width / 3, 260, 320);
    const RECT left_panel{hero.left, content_top, hero.left + left_width, content_bottom};
    const RECT right_panel{left_panel.right + gap, content_top, hero.right, content_bottom};
    const int action_y = content_bottom - 54;

    const int top_btn_w = 64;
    const int top_btn_h = 28;
    const int top_btn_y = hero.top + (hero_height - top_btn_h) / 2;
    MoveWindow(GetDlgItem(hwnd_, kIdButtonRestore), hero.right - 16 - top_btn_w, top_btn_y, top_btn_w, top_btn_h, TRUE);
    MoveWindow(GetDlgItem(hwnd_, kIdButtonBackup), hero.right - 16 - top_btn_w * 2 - 8, top_btn_y, top_btn_w, top_btn_h, TRUE);

    MoveWindow(meta_label_, hero.left + 16, hero.top + 22, hero.right - hero.left - 16 - top_btn_w * 2 - 8 - 16, 18, TRUE);

    MoveWindow(favorites_label_, left_panel.left + 16, left_panel.top + 16, left_panel.right - left_panel.left - 32, 18, TRUE);
    
    const int list_y = left_panel.top + 42;
    const int list_height = action_y - 10 - list_y;
    MoveWindow(list_box_, left_panel.left + 12, list_y, left_panel.right - left_panel.left - 24, list_height, TRUE);
    
    const int move_w = (left_panel.right - left_panel.left - 24 - 10) / 2;
    MoveWindow(GetDlgItem(hwnd_, kIdButtonMoveUp), left_panel.left + 12, action_y, move_w, 34, TRUE);
    MoveWindow(GetDlgItem(hwnd_, kIdButtonMoveDown), left_panel.left + 12 + move_w + 10, action_y, move_w, 34, TRUE);

    const int field_left = right_panel.left + 16;
    const int field_width = right_panel.right - right_panel.left - 32;
    const int search_button_width = 120;
    const int edit_height = 34;
    const int label_gap = 14;
    int y = right_panel.top + 16;

    if (show_status) {
        MoveWindow(status_label_, field_left, y, field_width, 18, TRUE);
        y += 28;
    }

    MoveWindow(name_label_, field_left, y, field_width, 18, TRUE);
    y += 24;
    MoveWindow(label_edit_, field_left, y, field_width, edit_height, TRUE);
    y += edit_height + label_gap;

    MoveWindow(kind_label_, field_left, y, field_width, 18, TRUE);
    y += 24;
    MoveWindow(kind_combo_, field_left, y, field_width, 220, TRUE);
    y += edit_height + label_gap;

    MoveWindow(target_label_, field_left, y, field_width, 18, TRUE);
    y += 24;
    MoveWindow(target_edit_, field_left, y, field_width - search_button_width - 10, edit_height, TRUE);
    MoveWindow(search_button_, field_left + field_width - search_button_width, y, search_button_width, edit_height, TRUE);

    MoveWindow(save_button_, field_left, action_y, 124, 34, TRUE);
    MoveWindow(reset_button_, field_left + 136, action_y, 110, 34, TRUE);
    MoveWindow(delete_button_, field_left + 258, action_y, 110, 34, TRUE);
}

void SettingsWindow::UpdateStatus(const std::wstring& message) {
    SetWindowTextW(status_label_, message.c_str());
    ShowWindow(status_label_, message.empty() ? SW_HIDE : SW_SHOW);
    LayoutControls();
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void SettingsWindow::ReloadList() {
    const std::wstring selected_id = editing_id_;
    SendMessageW(list_box_, LB_RESETCONTENT, 0, 0);

    int selected_index = LB_ERR;
    for (std::size_t index = 0; index < app_->state().favorites.size(); ++index) {
        const FavoriteItem& favorite = app_->state().favorites[index];
        const std::wstring text = favorite.label + L"  [" + KindDisplayName(favorite.kind) + L"]";
        SendMessageW(list_box_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        if (!selected_id.empty() && favorite.id == selected_id) {
            selected_index = static_cast<int>(index);
        }
    }

    if (selected_index != LB_ERR) {
        SendMessageW(list_box_, LB_SETCURSEL, static_cast<WPARAM>(selected_index), 0);
    }
}

void SettingsWindow::LoadSelectionIntoForm() {
    const int index = static_cast<int>(SendMessageW(list_box_, LB_GETCURSEL, 0, 0));
    if (index == LB_ERR || index >= static_cast<int>(app_->state().favorites.size())) {
        return;
    }

    const FavoriteItem& favorite = app_->state().favorites[static_cast<std::size_t>(index)];
    editing_id_ = favorite.id;
    SetWindowTextW(label_edit_, favorite.label.c_str());
    SetComboKind(favorite.kind);
    SetWindowTextW(target_edit_, favorite.target.c_str());
    SetWindowTextW(save_button_, L"\uBCC0\uACBD \uC800\uC7A5");

    const bool is_divider = CurrentKind() == L"divider";
    EnableWindow(target_edit_, !is_divider);
    EnableWindow(search_button_, !is_divider);
    UpdateStatus(L"");
}

void SettingsWindow::ResetForm() {
    editing_id_.clear();
    SetWindowTextW(label_edit_, L"");
    SetWindowTextW(target_edit_, L"");
    SendMessageW(kind_combo_, CB_SETCURSEL, 0, 0);
    SendMessageW(list_box_, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    SetWindowTextW(save_button_, L"\uC800\uC7A5");

    EnableWindow(target_edit_, TRUE);
    EnableWindow(search_button_, TRUE);
}

void SettingsWindow::UpdateMetaText() {
    const auto& widget = app_->state().widget;
    const std::wstring text =
        L"\uB4F1\uB85D " + std::to_wstring(app_->state().favorites.size()) +
        L"\uAC1C  \u00B7  \uC704\uC84B " + std::to_wstring(widget.x) +
        L", " + std::to_wstring(widget.y);
    SetWindowTextW(meta_label_, text.c_str());
}

std::wstring SettingsWindow::ReadWindowText(HWND control) const {
    const int length = GetWindowTextLengthW(control);
    std::wstring value(static_cast<std::size_t>(length + 1), L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

void SettingsWindow::SetComboKind(const std::wstring& kind) {
    const std::wstring normalized = NormalizeKind(kind);
    if (normalized == L"document") {
        SendMessageW(kind_combo_, CB_SETCURSEL, 1, 0);
    } else if (normalized == L"url") {
        SendMessageW(kind_combo_, CB_SETCURSEL, 2, 0);
    } else if (normalized == L"divider") {
        SendMessageW(kind_combo_, CB_SETCURSEL, 3, 0);
    } else {
        SendMessageW(kind_combo_, CB_SETCURSEL, 0, 0);
    }
}

std::wstring SettingsWindow::CurrentKind() const {
    const int selection = static_cast<int>(SendMessageW(kind_combo_, CB_GETCURSEL, 0, 0));
    if (selection == 1) {
        return L"document";
    }
    if (selection == 2) {
        return L"url";
    }
    if (selection == 3) {
        return L"divider";
    }
    return L"app";
}

void SettingsWindow::OpenSearchDialog() {
    if (CurrentKind() == L"url") {
        UpdateStatus(L"URL \uD56D\uBAA9\uC740 \uC9C1\uC811 \uC785\uB825\uD558\uC138\uC694.");
        return;
    }

    if (search_dialog_ != nullptr && IsWindow(search_dialog_->hwnd())) {
        search_dialog_->Show();
        return;
    }

    search_dialog_ = new SearchDialogWindow(this, CurrentKind(), ReadWindowText(label_edit_));
    if (!search_dialog_->Create(app_->instance(), hwnd_)) {
        delete search_dialog_;
        search_dialog_ = nullptr;
        UpdateStatus(L"\uAC80\uC0C9 \uCC3D\uC744 \uC5F4\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.");
        return;
    }

    search_dialog_->Show();
}

void SettingsWindow::CloseSearchDialog() {
    if (search_dialog_ != nullptr && IsWindow(search_dialog_->hwnd())) {
        DestroyWindow(search_dialog_->hwnd());
    }
    search_dialog_ = nullptr;
}

void SettingsWindow::HandleSave() {
    const std::wstring label = ReadWindowText(label_edit_);
    const std::wstring kind = CurrentKind();
    const std::wstring target = ReadWindowText(target_edit_);
    std::wstring error;

    bool ok = false;
    if (editing_id_.empty()) {
        ok = app_->CreateFavorite(label, kind, target, error);
    } else {
        ok = app_->UpdateFavorite(editing_id_, label, kind, target, error);
    }

    if (!ok) {
        UpdateStatus(error);
        return;
    }

    ResetForm();
    Refresh();
    UpdateStatus(L"\uC800\uC7A5\uD588\uC2B5\uB2C8\uB2E4.");
}

void SettingsWindow::HandleDelete() {
    if (editing_id_.empty()) {
        UpdateStatus(L"\uC0AD\uC81C\uD560 \uD56D\uBAA9\uC744 \uBA3C\uC800 \uC120\uD0DD\uD558\uC138\uC694.");
        return;
    }

    if (MessageBoxW(hwnd_, L"\uC120\uD0DD\uD55C \uD56D\uBAA9\uC744 \uC0AD\uC81C\uD558\uACA0\uC2B5\uB2C8\uAE4C?", kSettingsTitleText, MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    std::wstring error;
    if (!app_->DeleteFavorite(editing_id_, error)) {
        UpdateStatus(error);
        return;
    }

    ResetForm();
    Refresh();
    UpdateStatus(L"\uC0AD\uC81C\uD588\uC2B5\uB2C8\uB2E4.");
}

SearchDialogWindow::SearchDialogWindow(SettingsWindow* owner, const std::wstring& kind, const std::wstring& initial_query)
    : owner_(owner), kind_(kind), initial_query_(initial_query) {}

SearchDialogWindow::~SearchDialogWindow() {
    cancel_search_.store(true);
    FinishSearch();
    pending_output_.reset();
    if (detail_font_ != nullptr) {
        DeleteObject(detail_font_);
    }
    if (ui_font_ != nullptr) {
        DeleteObject(ui_font_);
    }
}

bool SearchDialogWindow::Create(HINSTANCE instance, HWND parent) {
    if (!Register(instance)) {
        return false;
    }

    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        ClassName(),
        kSearchTitleText,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        780,
        540,
        parent,
        nullptr,
        instance,
        this);

    if (hwnd_ != nullptr) {
        EnableModernWindowChrome(hwnd_);
    }

    return hwnd_ != nullptr;
}

void SearchDialogWindow::Show() {
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd_);
}

const wchar_t* SearchDialogWindow::ClassName() const {
    return L"FavoriteWidgetSearchDialog";
}

HBRUSH SearchDialogWindow::BackgroundBrush() const {
    return WindowBrush();
}

LRESULT SearchDialogWindow::HandleMessage(UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
    case WM_CREATE:
        CreateFonts();
        CreateControls();
        LayoutControls();
        if (!initial_query_.empty()) {
            SetWindowTextW(query_edit_, initial_query_.c_str());
            PostMessageW(hwnd_, WM_COMMAND, MAKEWPARAM(kIdSearchButton, BN_CLICKED), reinterpret_cast<LPARAM>(search_button_));
        }
        return 0;

    case WM_SIZE:
        LayoutControls();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd_, &paint);
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

        RECT hero = rect;
        hero.left += 12;
        hero.right -= 12;
        hero.top += 12;
        hero.bottom = hero.top + 54;
        DrawRoundedRect(dc, hero, kPanelBackground, kBorderColor, 0);

        RECT search_panel{hero.left, hero.bottom + 10, hero.right, hero.bottom + 110};
        RECT results_panel{hero.left, search_panel.bottom + 10, hero.right, rect.bottom - 12};
        DrawRoundedRect(dc, search_panel, ScaleColor(kPanelBackground, 0.98), kBorderColor, 0);
        DrawRoundedRect(dc, results_panel, ScaleColor(kPanelBackground, 0.96), kBorderColor, 0);

        RECT accent_strip = hero;
        accent_strip.left += 12;
        accent_strip.top += 10;
        accent_strip.right = accent_strip.left + 54;
        accent_strip.bottom = accent_strip.top + 2;
        DrawRoundedRect(dc, accent_strip, kAccentColor, kAccentColor, 0);

        RECT search_glow{search_panel.left + 12, search_panel.top + 10, search_panel.left + 54, search_panel.top + 12};
        RECT results_glow{results_panel.left + 12, results_panel.top + 10, results_panel.left + 54, results_panel.top + 12};
        DrawRoundedRect(dc, search_glow, ScaleColor(kAccentColor, 0.9), ScaleColor(kAccentColor, 0.9), 0);
        DrawRoundedRect(dc, results_glow, ScaleColor(kAccentColor, 0.9), ScaleColor(kAccentColor, 0.9), 0);

        EndPaint(hwnd_, &paint);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(w_param);
        const HWND control = reinterpret_cast<HWND>(l_param);
        SetTextColor(dc, (control == status_label_ || control == query_label_ || control == results_label_) ? kTextMuted : kTextPrimary);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(WindowBrush());
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(w_param);
        SetTextColor(dc, kTextPrimary);
        SetBkColor(dc, kPanelElevated);
        return reinterpret_cast<LRESULT>(ElevatedBrush());
    }

    case WM_DRAWITEM:
        DrawStyledButton(reinterpret_cast<const DRAWITEMSTRUCT*>(l_param), ui_font_, nullptr);
        return TRUE;

    case WM_COMMAND: {
        const WORD control_id = LOWORD(w_param);
        const WORD notify_code = HIWORD(w_param);

        if (control_id == kIdSearchButton) {
            StartSearch();
            return 0;
        }
        if (control_id == kIdSearchSelect) {
            AcceptSelection();
            return 0;
        }
        if (control_id == kIdSearchCancel) {
            DestroyWindow(hwnd_);
            return 0;
        }
        if (control_id == kIdSearchResults && notify_code == LBN_DBLCLK) {
            AcceptSelection();
            return 0;
        }
        return 0;
    }

    case kMessageSearchComplete: {
        std::unique_ptr<SearchOutput> output;
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            output = std::move(pending_output_);
        }

        FinishSearch();
        SendMessageW(results_list_, LB_RESETCONTENT, 0, 0);

        if (output == nullptr) {
            SetWindowTextW(status_label_, L"\uAC80\uC0C9 \uACB0\uACFC\uB97C \uAC00\uC838\uC624\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4.");
            return 0;
        }

        for (const auto& result : output->results) {
            SendMessageW(results_list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(result.c_str()));
        }

        if (!output->error.empty()) {
            SetWindowTextW(status_label_, output->error.c_str());
        } else if (output->results.empty()) {
            const std::wstring text = output->source.empty()
                ? L"\uACB0\uACFC\uB97C \uCC3E\uC9C0 \uBABB\uD588\uC2B5\uB2C8\uB2E4."
                : output->source + L" \uACB0\uACFC\uAC00 \uC5C6\uC2B5\uB2C8\uB2E4.";
            SetWindowTextW(status_label_, text.c_str());
        } else {
            const std::wstring text = output->source + L" \uACB0\uACFC " + std::to_wstring(output->results.size()) + L"\uAC1C";
            SetWindowTextW(status_label_, text.c_str());
            SendMessageW(results_list_, LB_SETCURSEL, 0, 0);
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd_);
        return 0;

    case WM_DESTROY:
        cancel_search_.store(true);
        FinishSearch();
        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            pending_output_.reset();
        }
        return 0;

    case WM_NCDESTROY:
        if (owner_ != nullptr) {
            owner_->OnSearchDialogClosed();
        }
        delete this;
        return 0;
    }

    return DefWindowProcW(hwnd_, message, w_param, l_param);
}

void SearchDialogWindow::CreateFonts() {
    ui_font_ = CreateUiFont(12, FW_SEMIBOLD);
    detail_font_ = CreateUiFont(11, FW_NORMAL, L"Consolas");
}

void SearchDialogWindow::CreateControls() {
    const HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd_, GWLP_HINSTANCE));

    query_label_ = CreateWindowExW(0, L"STATIC", L"\uAC80\uC0C9\uC5B4", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    query_edit_ = CreateWindowExW(
        0,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdSearchQuery),
        instance,
        nullptr);
    status_label_ = CreateWindowExW(
        0,
        L"STATIC",
        L"\uAC80\uC0C9\uC5B4\uB97C \uC785\uB825\uD558\uACE0 \uAC80\uC0C9\uC744 \uB204\uB974\uC138\uC694.",
        WS_CHILD | WS_VISIBLE,
        0,
        0,
        0,
        0,
        hwnd_,
        nullptr,
        instance,
        nullptr);
    results_label_ = CreateWindowExW(0, L"STATIC", L"\uAC80\uC0C9 \uACB0\uACFC", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance, nullptr);
    results_list_ = CreateWindowExW(
        0,
        L"LISTBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(kIdSearchResults),
        instance,
        nullptr);
    search_button_ = CreateWindowExW(0, L"BUTTON", L"\uAC80\uC0C9", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdSearchButton), instance, nullptr);
    HWND select_button = CreateWindowExW(0, L"BUTTON", L"\uC120\uD0DD", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdSearchSelect), instance, nullptr);
    HWND cancel_button = CreateWindowExW(0, L"BUTTON", L"\uB2EB\uAE30", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(kIdSearchCancel), instance, nullptr);

    for (HWND control : {query_label_, status_label_, results_label_}) {
        ApplyFont(control, detail_font_);
    }

    for (HWND control : {query_edit_, results_list_, search_button_, select_button, cancel_button}) {
        ApplyFont(control, ui_font_);
    }

    SendMessageW(query_edit_, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"\uD30C\uC77C \uC774\uB984 \uAC80\uC0C9"));
    SendMessageW(query_edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(10, 10));
    SendMessageW(results_list_, LB_SETITEMHEIGHT, 0, 34);
}

void SearchDialogWindow::LayoutControls() {
    RECT client{};
    GetClientRect(hwnd_, &client);

    const int margin = 12;
    const int hero_height = 54;
    const int hero_top = 12;
    const RECT hero{margin, hero_top, client.right - margin, hero_top + hero_height};
    const RECT search_panel{hero.left, hero.bottom + 10, hero.right, hero.bottom + 110};
    const RECT results_panel{hero.left, search_panel.bottom + 10, hero.right, client.bottom - margin};
    const int button_width = 96;
    const int field_left = search_panel.left + 16;
    const int field_width = search_panel.right - search_panel.left - 32;
    const int input_y = search_panel.top + 24;

    MoveWindow(query_label_, field_left, search_panel.top + 16, field_width, 18, TRUE);
    MoveWindow(query_edit_, field_left, input_y, field_width - button_width - 10, 34, TRUE);
    MoveWindow(search_button_, field_left + field_width - button_width, input_y, button_width, 34, TRUE);
    MoveWindow(status_label_, field_left, input_y + 44, field_width, 18, TRUE);

    MoveWindow(results_label_, results_panel.left + 16, results_panel.top + 16, results_panel.right - results_panel.left - 32, 18, TRUE);
    MoveWindow(results_list_, results_panel.left + 12, results_panel.top + 40, results_panel.right - results_panel.left - 24, results_panel.bottom - results_panel.top - 90, TRUE);

    HWND select_button = GetDlgItem(hwnd_, kIdSearchSelect);
    HWND cancel_button = GetDlgItem(hwnd_, kIdSearchCancel);
    MoveWindow(select_button, results_panel.right - 212, results_panel.bottom - 44, 100, 34, TRUE);
    MoveWindow(cancel_button, results_panel.right - 100, results_panel.bottom - 44, 96, 34, TRUE);
}

void SearchDialogWindow::StartSearch() {
    if (search_running_) {
        SetWindowTextW(status_label_, L"\uC774\uBBF8 \uAC80\uC0C9 \uC911\uC785\uB2C8\uB2E4.");
        return;
    }

    const std::wstring query = ReadWindowText(query_edit_);
    if (Trim(query).empty()) {
        SetWindowTextW(status_label_, L"\uAC80\uC0C9\uC5B4\uB97C \uC785\uB825\uD558\uC138\uC694.");
        return;
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    {
        std::lock_guard<std::mutex> lock(result_mutex_);
        pending_output_.reset();
    }

    SendMessageW(results_list_, LB_RESETCONTENT, 0, 0);
    SetWindowTextW(status_label_, L"\uAC80\uC0C9 \uC911...");
    EnableWindow(search_button_, FALSE);

    search_running_ = true;
    cancel_search_.store(false);

    const HWND target_hwnd = hwnd_;
    const std::wstring kind = kind_;
    worker_ = std::thread([this, target_hwnd, query, kind]() {
        auto output = std::make_unique<SearchOutput>(RunSearch(query, kind, &cancel_search_));
        if (cancel_search_.load()) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(result_mutex_);
            pending_output_ = std::move(output);
        }

        if (IsWindow(target_hwnd)) {
            PostMessageW(target_hwnd, kMessageSearchComplete, 0, 0);
        }
    });
}

void SearchDialogWindow::FinishSearch() {
    if (worker_.joinable()) {
        worker_.join();
    }
    search_running_ = false;
    if (search_button_ != nullptr && IsWindow(search_button_)) {
        EnableWindow(search_button_, TRUE);
    }
}

void SearchDialogWindow::AcceptSelection() {
    const int index = static_cast<int>(SendMessageW(results_list_, LB_GETCURSEL, 0, 0));
    if (index == LB_ERR) {
        SetWindowTextW(status_label_, L"\uC120\uD0DD\uD560 \uACB0\uACFC\uAC00 \uC5C6\uC2B5\uB2C8\uB2E4.");
        return;
    }

    const int length = static_cast<int>(SendMessageW(results_list_, LB_GETTEXTLEN, index, 0));
    std::wstring value(static_cast<std::size_t>(length + 1), L'\0');
    SendMessageW(results_list_, LB_GETTEXT, index, reinterpret_cast<LPARAM>(value.data()));
    value.resize(static_cast<std::size_t>(length));

    if (owner_ != nullptr) {
        owner_->SetSearchResult(value);
    }
    DestroyWindow(hwnd_);
}

std::wstring SearchDialogWindow::ReadWindowText(HWND control) const {
    const int length = GetWindowTextLengthW(control);
    std::wstring value(static_cast<std::size_t>(length + 1), L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

FavoriteApp::FavoriteApp(HINSTANCE instance, bool smoke_test)
    : instance_(instance), smoke_test_(smoke_test) {}

FavoriteApp::~FavoriteApp() {
    ShutdownUiAssets();
}

bool FavoriteApp::Initialize() {
    InitializeUiAssets();
    state_ = storage_.Load();
    main_window_ = std::make_unique<MainWidgetWindow>(this);
    return main_window_->Create(instance_);
}

int FavoriteApp::Run() {
    main_window_->Show();

    if (smoke_test_) {
        SetTimer(main_window_->hwnd(), 1, 500, nullptr);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

HWND FavoriteApp::main_hwnd() const {
    return main_window_ != nullptr ? main_window_->hwnd() : nullptr;
}

void FavoriteApp::OpenSettingsWindow() {
    if (settings_window_ == nullptr) {
        settings_window_ = new SettingsWindow(this);
        if (!settings_window_->Create(instance_, main_hwnd())) {
            delete settings_window_;
            settings_window_ = nullptr;
            return;
        }
    }

    settings_window_->Refresh();
    settings_window_->Show();
}

void FavoriteApp::OnSettingsClosed() {
    settings_window_ = nullptr;
}

bool FavoriteApp::CreateFavorite(const std::wstring& label, const std::wstring& kind, const std::wstring& target, std::wstring& error) {
    FavoriteItem normalized;
    if (!NormalizeFavoriteInput(label, kind, target, normalized, error)) {
        return false;
    }

    normalized.id = GenerateId();
    state_.favorites.push_back(normalized);
    if (!Persist(&error)) {
        state_.favorites.pop_back();
        return false;
    }

    RefreshAllWindows();
    return true;
}

bool FavoriteApp::UpdateFavorite(const std::wstring& id, const std::wstring& label, const std::wstring& kind, const std::wstring& target, std::wstring& error) {
    FavoriteItem normalized;
    if (!NormalizeFavoriteInput(label, kind, target, normalized, error)) {
        return false;
    }

    for (auto& favorite : state_.favorites) {
        if (favorite.id == id) {
            const FavoriteItem backup = favorite;
            favorite.label = normalized.label;
            favorite.kind = normalized.kind;
            favorite.target = normalized.target;

            if (!Persist(&error)) {
                favorite = backup;
                return false;
            }

            RefreshAllWindows();
            return true;
        }
    }

    error = L"\uC218\uC815\uD560 \uD56D\uBAA9\uC744 \uCC3E\uC744 \uC218 \uC5C6\uC2B5\uB2C8\uB2E4.";
    return false;
}

bool FavoriteApp::DeleteFavorite(const std::wstring& id, std::wstring& error) {
    const auto backup = state_.favorites;
    state_.favorites.erase(
        std::remove_if(state_.favorites.begin(), state_.favorites.end(), [&](const FavoriteItem& favorite) {
            return favorite.id == id;
        }),
        state_.favorites.end());

    if (state_.favorites.size() == backup.size()) {
        error = L"\uC0AD\uC81C\uD560 \uD56D\uBAA9\uC744 \uCC3E\uC744 \uC218 \uC5C6\uC2B5\uB2C8\uB2E4.";
        return false;
    }

    if (!Persist(&error)) {
        state_.favorites = backup;
        return false;
    }

    RefreshAllWindows();
    return true;
}

bool FavoriteApp::LaunchFavorite(std::size_t index, std::wstring& error) {
    if (index >= state_.favorites.size()) {
        error = L"\uC2E4\uD589\uD560 \uD56D\uBAA9\uC744 \uCC3E\uC744 \uC218 \uC5C6\uC2B5\uB2C8\uB2E4.";
        return false;
    }

    const FavoriteItem& favorite = state_.favorites[index];
    if (favorite.kind.empty() || favorite.target.empty()) {
        error = L"\uC2E4\uD589\uD560 \uD56D\uBAA9\uC744 \uCC3E\uC744 \uC218 \uC5C6\uC2B5\uB2C8\uB2E4.";
        return false;
    }

    return LaunchTarget(favorite, error);
}

void FavoriteApp::UpdateWidgetPosition(int x, int y) {
    state_.widget.x = x;
    state_.widget.y = y;
    Persist(nullptr);

    if (settings_window_ != nullptr) {
        settings_window_->Refresh();
    }
}

void FavoriteApp::RefreshAllWindows() {
    if (main_window_ != nullptr) {
        main_window_->Refresh();
        KeepWidgetOnTop(main_window_->hwnd());
    }

    if (settings_window_ != nullptr) {
        settings_window_->Refresh();
    }
}

bool FavoriteApp::Persist(std::wstring* error) {
    return storage_.Save(state_, error);
}
