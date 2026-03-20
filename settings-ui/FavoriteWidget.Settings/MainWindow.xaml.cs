using FavoriteWidget.Settings.Models;
using FavoriteWidget.Settings.Services;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.Storage.Pickers;
using WinRT.Interop;

namespace FavoriteWidget.Settings;

public sealed partial class MainWindow : Window {
    private readonly IniFavoriteService _service = new();
    private readonly ObservableCollection<FavoriteItem> _items = new();
    private string? _editingId;

    public MainWindow() {
        InitializeComponent();
        FavoritesList.ItemsSource = _items;
        UpdateFormForNoSelection();
        Reload();
    }

    private void OnFavoriteSelectionChanged(object sender, SelectionChangedEventArgs e) {
        if (FavoritesList.SelectedItem is not FavoriteItem selected) {
            UpdateFormForNoSelection();
            return;
        }

        _editingId = selected.Id;
        LabelTextBox.Text = selected.Label;
        TargetTextBox.Text = selected.Target;
        SetKind(selected.Kind);
        SaveButton.Label = "Update";
        DeleteButton.IsEnabled = true;
        ShowStatus(string.Empty);
    }

    private void OnNewClicked(object sender, RoutedEventArgs e) {
        UpdateFormForNoSelection();
        ShowStatus("Create mode. Enter values and press Save.");
    }

    private void OnRefreshClicked(object sender, RoutedEventArgs e) {
        Reload();
        ShowStatus("Reloaded from disk.");
    }

    private void OnSaveClicked(object sender, RoutedEventArgs e) {
        var isCreating = string.IsNullOrEmpty(_editingId);
        var label = LabelTextBox.Text ?? string.Empty;
        var kind = GetSelectedKind();
        var target = TargetTextBox.Text ?? string.Empty;

        string error;
        var ok = isCreating
            ? _service.TryCreateFavorite(label, kind, target, out error)
            : _service.TryUpdateFavorite(_editingId, label, kind, target, out error);

        if (!ok) {
            ShowStatus(error, isError: true);
            return;
        }

        Reload();
        UpdateFormForNoSelection();
        ShowStatus(isCreating ? "Added." : "Updated.");
    }

    private async void OnDeleteClicked(object sender, RoutedEventArgs e) {
        if (string.IsNullOrEmpty(_editingId)) {
            ShowStatus("No item selected to delete.", isError: true);
            return;
        }

        var selected = _items.FirstOrDefault(item => item.Id == _editingId);
        if (selected == null) {
            _editingId = null;
            UpdateFormForNoSelection();
            ShowStatus("Selected item no longer exists.", isError: true);
            return;
        }

        var dialog = new ContentDialog {
            Title = "Delete favorite",
            Content = $"Delete '{selected.Label}'?",
            PrimaryButtonText = "Delete",
            CloseButtonText = "Cancel",
            DefaultButton = ContentDialogButton.Primary,
            XamlRoot = XamlRoot,
        };

        var result = await dialog.ShowAsync();
        if (result != ContentDialogResult.Primary) {
            return;
        }

        if (!_service.TryDeleteFavorite(_editingId, out var error)) {
            ShowStatus(error, isError: true);
            return;
        }

        Reload();
        UpdateFormForNoSelection();
        ShowStatus("Deleted.");
    }

    private async void OnPickTargetClicked(object sender, RoutedEventArgs e) {
        var kind = GetSelectedKind();
        if (kind == "url") {
            ShowStatus("URL type does not require path picker.", isError: true);
            return;
        }

        var picker = new FileOpenPicker();
        var hwnd = WindowNative.GetWindowHandle(this);
        InitializeWithWindow.Initialize(picker, hwnd);

        picker.SuggestedStartLocation = PickerLocationId.ComputerFolder;
        if (kind == "app") {
            picker.FileTypeFilter.Add(".exe");
        } else {
            picker.FileTypeFilter.Add("*");
        }

        var file = await picker.PickSingleFileAsync();
        if (file is not null) {
            TargetTextBox.Text = file.Path;
            ShowStatus(string.Empty);
        }
    }

    private void OnKindSelectionChanged(object sender, SelectionChangedEventArgs e) {
        var kind = GetSelectedKind();
        PickTargetButton.IsEnabled = kind != "url";

        if (kind == "url") {
            ShowStatus("URL mode: type or paste URL directly.", isError: true);
            return;
        }

        StatusText.Visibility = Visibility.Collapsed;
    }

    private void Reload() {
        var selectedId = _editingId;

        var state = _service.Load();
        _items.Clear();
        foreach (var favorite in state.Favorites) {
            _items.Add(favorite);
        }

        UpdateMeta(state);

        if (!string.IsNullOrEmpty(selectedId)) {
            var selectedItem = _items.FirstOrDefault(item => item.Id == selectedId);
            if (selectedItem != null) {
                FavoritesList.SelectedItem = selectedItem;
                return;
            }
        }

        FavoritesList.SelectedItem = null;
        _editingId = null;

        if (_items.Count == 0) {
            EmptyState.Visibility = Visibility.Visible;
            FavoritesList.Visibility = Visibility.Collapsed;
        } else {
            EmptyState.Visibility = Visibility.Collapsed;
            FavoritesList.Visibility = Visibility.Visible;
        }

        UpdateFormForNoSelection();
    }

    private void UpdateMeta(SettingsState state) {
        MetaLabel.Text = $"Total {state.Favorites.Count} items / position {state.Widget.X}, {state.Widget.Y}";
    }

    private void UpdateFormForNoSelection() {
        _editingId = null;
        LabelTextBox.Text = string.Empty;
        TargetTextBox.Text = string.Empty;
        SetKind("app");
        SaveButton.Label = "Save";
        DeleteButton.IsEnabled = false;
        FavoritesList.SelectedItem = null;
    }

    private void SetKind(string kind) {
        var index = kind switch {
            "document" => 1,
            "url" => 2,
            _ => 0,
        };

        if (index >= 0 && index < KindCombo.Items.Count) {
            KindCombo.SelectedIndex = index;
        } else {
            KindCombo.SelectedIndex = 0;
        }
    }

    private string GetSelectedKind() {
        if (KindCombo.SelectedItem is not ComboBoxItem item) {
            return "app";
        }

        if (item.Tag is string tag && !string.IsNullOrWhiteSpace(tag)) {
            return tag;
        }

        return item.Content?.ToString()?.ToLowerInvariant() switch {
            "url" => "url",
            "document/file" => "document",
            _ => "app",
        };
    }

    private void ShowStatus(string message, bool isError = false) {
        StatusText.Text = message;
        if (string.IsNullOrWhiteSpace(message)) {
            StatusText.Visibility = Visibility.Collapsed;
            return;
        }

        StatusText.Visibility = Visibility.Visible;
        StatusText.Foreground = new SolidColorBrush(
            isError
                ? Windows.UI.Colors.Firebrick
                : Windows.UI.Colors.DodgerBlue);
    }
}
