#pragma once

#include "model.hpp"
#include "storage.hpp"
#include "ui_assets.hpp"

#include <Windows.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

struct SearchOutput;

constexpr int kWidgetWidth = 472;
constexpr int kWidgetHeight = 410;
constexpr int kSettingsWidth = 940;
constexpr int kSettingsHeight = 620;

class FavoriteApp;
class SettingsWindow;
class SearchDialogWindow;

class WindowBase {
public:
    virtual ~WindowBase() = default;
    HWND hwnd() const { return hwnd_; }

protected:
    HWND hwnd_ = nullptr;
    virtual LRESULT HandleMessage(UINT message, WPARAM w_param, LPARAM l_param) = 0;
    virtual const wchar_t* ClassName() const = 0;
    virtual DWORD ClassStyle() const { return CS_HREDRAW | CS_VREDRAW; }
    virtual HBRUSH BackgroundBrush() const { return reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); }

    bool Register(HINSTANCE instance) const;
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);
};

class MainWidgetWindow : public WindowBase {
public:
    explicit MainWidgetWindow(FavoriteApp* app);
    ~MainWidgetWindow() override;
    bool Create(HINSTANCE instance);
    void Show();
    void Refresh();
    void ShowError(const std::wstring& message);

protected:
    const wchar_t* ClassName() const override;
    HBRUSH BackgroundBrush() const override;
    LRESULT HandleMessage(UINT message, WPARAM w_param, LPARAM l_param) override;

private:
    FavoriteApp* app_;
    HFONT ui_font_ = nullptr;
    HFONT title_font_ = nullptr;
    HFONT detail_font_ = nullptr;
    HWND title_label_ = nullptr;
    HWND subtitle_label_ = nullptr;
    HWND count_label_ = nullptr;
    HWND settings_button_ = nullptr;
    HWND close_button_ = nullptr;
    HWND status_label_ = nullptr;
    std::vector<HWND> favorite_buttons_;
    int scroll_offset_ = 0;

    void CreateFonts();
    void CreateControls();
    void DestroyDynamicButtons();
    void LayoutControls();
    void RebuildButtons();
    void UpdateScrollBar();
    void ScrollTo(int offset);
    std::optional<std::size_t> HitFavoriteIndex(WORD control_id) const;
};

class SettingsWindow : public WindowBase {
public:
    explicit SettingsWindow(FavoriteApp* app);
    ~SettingsWindow() override;

    bool Create(HINSTANCE instance, HWND owner);
    void Show();
    void Refresh();
    void SetSearchResult(const std::wstring& path);
    void OnSearchDialogClosed();

protected:
    const wchar_t* ClassName() const override;
    HBRUSH BackgroundBrush() const override;
    LRESULT HandleMessage(UINT message, WPARAM w_param, LPARAM l_param) override;

private:
    FavoriteApp* app_;
    HFONT ui_font_ = nullptr;
    HFONT detail_font_ = nullptr;
    HWND owner_ = nullptr;
    HWND meta_label_ = nullptr;
    HWND status_label_ = nullptr;
    HWND favorites_label_ = nullptr;
    HWND list_box_ = nullptr;
    HWND name_label_ = nullptr;
    HWND label_edit_ = nullptr;
    HWND kind_label_ = nullptr;
    HWND kind_combo_ = nullptr;
    HWND target_label_ = nullptr;
    HWND target_edit_ = nullptr;
    HWND search_button_ = nullptr;
    HWND save_button_ = nullptr;
    HWND reset_button_ = nullptr;
    HWND delete_button_ = nullptr;
    std::wstring editing_id_;
    SearchDialogWindow* search_dialog_ = nullptr;

    void CreateFonts();
    void CreateControls();
    void LayoutControls();
    void UpdateStatus(const std::wstring& message);
    void ReloadList();
    void LoadSelectionIntoForm();
    void ResetForm();
    void UpdateMetaText();
    std::wstring ReadWindowText(HWND control) const;
    void SetComboKind(const std::wstring& kind);
    std::wstring CurrentKind() const;
    void OpenSearchDialog();
    void CloseSearchDialog();
    void HandleSave();
    void HandleDelete();
};

class SearchDialogWindow : public WindowBase {
public:
    SearchDialogWindow(SettingsWindow* owner, const std::wstring& kind, const std::wstring& initial_query);
    ~SearchDialogWindow() override;

    bool Create(HINSTANCE instance, HWND parent);
    void Show();

protected:
    const wchar_t* ClassName() const override;
    HBRUSH BackgroundBrush() const override;
    LRESULT HandleMessage(UINT message, WPARAM w_param, LPARAM l_param) override;

private:
    SettingsWindow* owner_;
    std::wstring kind_;
    std::wstring initial_query_;
    HFONT ui_font_ = nullptr;
    HFONT detail_font_ = nullptr;
    HWND query_label_ = nullptr;
    HWND query_edit_ = nullptr;
    HWND status_label_ = nullptr;
    HWND results_label_ = nullptr;
    HWND results_list_ = nullptr;
    HWND search_button_ = nullptr;
    bool search_running_ = false;
    std::atomic<bool> cancel_search_{false};
    std::thread worker_;
    std::mutex result_mutex_;
    std::unique_ptr<SearchOutput> pending_output_;

    void CreateFonts();
    void CreateControls();
    void LayoutControls();
    void StartSearch();
    void FinishSearch();
    void AcceptSelection();
    std::wstring ReadWindowText(HWND control) const;
};

class FavoriteApp {
public:
    FavoriteApp(HINSTANCE instance, bool smoke_test);
    ~FavoriteApp();

    bool Initialize();
    int Run();

    HINSTANCE instance() const { return instance_; }
    bool smoke_test() const { return smoke_test_; }
    const AppState& state() const { return state_; }
    HWND main_hwnd() const;
    SettingsWindow* settings_window() const { return settings_window_; }

    void OpenSettingsWindow();
    bool LaunchSettingsWindow();
    void ReloadFromStorage();
    void OnSettingsClosed();
    bool CreateFavorite(const std::wstring& label, const std::wstring& kind, const std::wstring& target, std::wstring& error);
    bool UpdateFavorite(const std::wstring& id, const std::wstring& label, const std::wstring& kind, const std::wstring& target, std::wstring& error);
    bool DeleteFavorite(const std::wstring& id, std::wstring& error);
    bool LaunchFavorite(std::size_t index, std::wstring& error);
    void UpdateWidgetPosition(int x, int y);
    void RefreshAllWindows();

private:
    HINSTANCE instance_;
    bool smoke_test_;
    std::wstring settings_app_path_;
    Storage storage_;
    AppState state_;
    std::unique_ptr<MainWidgetWindow> main_window_;
    SettingsWindow* settings_window_ = nullptr;

    HANDLE settings_process_handle_ = nullptr;
    std::thread settings_watcher_thread_;
    std::atomic<bool> settings_watcher_running_ = false;
    std::atomic<bool> settings_launch_in_progress_ = false;

    bool Persist(std::wstring* error = nullptr);
    bool ResolveSettingsExePath();
    bool LaunchSettingsFallback();
    void StartSettingsWatcher();
    void StopSettingsWatcher();
};
