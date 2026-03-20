namespace FavoriteWidget.Settings.Models;

public sealed class FavoriteItem {
    public string Id { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public string Kind { get; set; } = string.Empty;
    public string Target { get; set; } = string.Empty;

    public string KindLabel => Kind switch {
        "document" => "Document/File",
        "url" => "URL",
        _ => "Application",
    };
}

public sealed class WidgetState {
    public int X { get; set; } = 48;
    public int Y { get; set; } = 48;
}

public sealed class SettingsState {
    public WidgetState Widget { get; set; } = new();
    public System.Collections.Generic.List<FavoriteItem> Favorites { get; set; } = new();
}
