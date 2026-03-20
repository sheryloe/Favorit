using FavoriteWidget.Settings.Models;
using System.Globalization;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace FavoriteWidget.Settings.Services;

public sealed class IniFavoriteService {
    private const string WidgetSection = "Widget";
    private const string FavoriteSection = "Favorites";
    private const string CountKey = "Count";
    private const string IdKey = "Id";
    private const string LabelKey = "Label";
    private const string KindKey = "Kind";
    private const string TargetKey = "Target";

    public string FilePath { get; }

    public IniFavoriteService() {
        var localAppData = Environment.GetEnvironmentVariable("LOCALAPPDATA");
        if (string.IsNullOrWhiteSpace(localAppData)) {
            localAppData = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        }

        FilePath = Path.Combine(localAppData, "FavoriteWidget", "favorite-widget.ini");
    }

    public SettingsState Load() {
        var result = new SettingsState();
        result.Widget.X = ReadInt(WidgetSection, "X", 48);
        result.Widget.Y = ReadInt(WidgetSection, "Y", 48);

        var count = Math.Max(0, ReadInt(FavoriteSection, CountKey, 0));
        for (var index = 0; index < count; ++index) {
            var section = $"Favorite{index}";
            var id = ReadString(section, IdKey, string.Empty);
            var label = ReadString(section, LabelKey, string.Empty);
            var kind = ReadString(section, KindKey, string.Empty);
            var target = ReadString(section, TargetKey, string.Empty);

            if (string.IsNullOrWhiteSpace(id)
                || string.IsNullOrWhiteSpace(label)
                || string.IsNullOrWhiteSpace(kind)
                || string.IsNullOrWhiteSpace(target)) {
                continue;
            }

            result.Favorites.Add(new FavoriteItem {
                Id = id,
                Label = label,
                Kind = kind,
                Target = target,
            });
        }

        return result;
    }

    public bool Save(SettingsState state, out string error) {
        error = string.Empty;
        if (!TryEnsureDirectory(out error)) {
            return false;
        }

        var previousCount = ReadInt(FavoriteSection, CountKey, 0);
        for (var index = 0; index < previousCount; ++index) {
            if (!WriteString($"Favorite{index}", null, null)) {
                error = "Failed to clear existing favorites.";
                return false;
            }
        }

        if (!WriteString(WidgetSection, "X", ResultToString(state.Widget.X))) {
            error = "Failed to write widget X.";
            return false;
        }

        if (!WriteString(WidgetSection, "Y", ResultToString(state.Widget.Y))) {
            error = "Failed to write widget Y.";
            return false;
        }

        if (!WriteInt(FavoriteSection, CountKey, state.Favorites.Count)) {
            error = "Failed to write favorites count.";
            return false;
        }

        for (var index = 0; index < state.Favorites.Count; ++index) {
            var section = $"Favorite{index}";
            var favorite = state.Favorites[index];

            if (!WriteString(section, IdKey, favorite.Id)
                || !WriteString(section, LabelKey, favorite.Label)
                || !WriteString(section, KindKey, favorite.Kind)
                || !WriteString(section, TargetKey, favorite.Target)) {
                error = "Failed to write favorite item.";
                return false;
            }
        }

        return true;
    }

    public bool TryCreateFavorite(string label, string kind, string target, out string error) {
        if (!TryNormalizeInput(label, kind, target, out var normalized, out error)) {
            return false;
        }

        var state = Load();
        normalized.Id = Guid.NewGuid().ToString("B");
        state.Favorites.Add(normalized);

        if (!Save(state, out error)) {
            state.Favorites.Remove(normalized);
            return false;
        }

        return true;
    }

    public bool TryUpdateFavorite(string id, string label, string kind, string target, out string error) {
        if (!TryNormalizeInput(label, kind, target, out var normalized, out error)) {
            return false;
        }

        var state = Load();
        for (var index = 0; index < state.Favorites.Count; ++index) {
            if (state.Favorites[index].Id != id) {
                continue;
            }

            var backup = state.Favorites[index];
            state.Favorites[index].Label = normalized.Label;
            state.Favorites[index].Kind = normalized.Kind;
            state.Favorites[index].Target = normalized.Target;

            if (!Save(state, out error)) {
                state.Favorites[index] = backup;
                return false;
            }

            return true;
        }

        error = "Item to update was not found.";
        return false;
    }

    public bool TryDeleteFavorite(string id, out string error) {
        error = string.Empty;
        var state = Load();
        var previousCount = state.Favorites.Count;

        state.Favorites = state.Favorites
            .Where(item => item.Id != id)
            .ToList();

        if (previousCount == state.Favorites.Count) {
            error = "Item to delete was not found.";
            return false;
        }

        if (!Save(state, out error)) {
            return false;
        }

        return true;
    }

    private static string ResultToString(int value) => value.ToString(CultureInfo.InvariantCulture);

    public static bool TryNormalizeInput(
        string label,
        string kind,
        string target,
        out FavoriteItem normalized,
        out string error) {

        normalized = new FavoriteItem();
        error = string.Empty;

        label = label.Trim();
        target = target.Trim();
        kind = NormalizeKind(kind);

        if (string.IsNullOrWhiteSpace(label)) {
            error = "Label is required.";
            return false;
        }

        if (string.IsNullOrWhiteSpace(kind)) {
            error = "Item type is required.";
            return false;
        }

        if (string.IsNullOrWhiteSpace(target)) {
            error = "Target is required.";
            return false;
        }

        if (kind == "url") {
            target = StripWrappingQuotes(target);
            if (!IsValidUrl(target)) {
                error = "Only http/https links are allowed.";
                return false;
            }
        } else {
            target = StripWrappingQuotes(target);
            if (!TryNormalizeFilePath(target, kind, out var fullPath, out error)) {
                return false;
            }
            target = fullPath;
        }

        normalized = new FavoriteItem {
            Label = label,
            Kind = kind,
            Target = target,
        };

        return true;
    }

    private static string NormalizeKind(string kind) {
        var normalized = kind.Trim().ToLowerInvariant();
        return normalized switch {
            "document" => "document",
            "url" => "url",
            _ => string.IsNullOrWhiteSpace(normalized) || normalized == "app" ? "app" : string.Empty,
        };
    }

    private static bool IsValidUrl(string value) {
        return value.StartsWith("http://", StringComparison.OrdinalIgnoreCase)
            || value.StartsWith("https://", StringComparison.OrdinalIgnoreCase);
    }

    private static string StripWrappingQuotes(string value) {
        if (value.Length >= 2
            && ((value[0] == '"' && value[^1] == '"') || (value[0] == '\'' && value[^1] == '\''))) {
            return value[1..^1];
        }

        return value;
    }

    private static bool TryNormalizeFilePath(string input, string kind, out string fullPath, out string error) {
        error = string.Empty;
        fullPath = string.Empty;

        try {
            fullPath = Path.GetFullPath(input);
        } catch (Exception) {
            error = "Invalid file path format.";
            return false;
        }

        if (!File.Exists(fullPath)) {
            error = "The target file does not exist.";
            return false;
        }

        if (kind == "app" && !string.Equals(Path.GetExtension(fullPath), ".exe", StringComparison.OrdinalIgnoreCase)) {
            error = "Application favorites require an .exe file.";
            return false;
        }

        return true;
    }

    private bool TryEnsureDirectory(out string error) {
        error = string.Empty;
        var directory = Path.GetDirectoryName(FilePath);
        if (string.IsNullOrEmpty(directory)) {
            return true;
        }

        try {
            Directory.CreateDirectory(directory);
            return true;
        } catch (Exception ex) {
            error = ex.Message;
            return false;
        }
    }

    private int ReadInt(string section, string key, int fallback) {
        return GetPrivateProfileInt(section, key, fallback, FilePath);
    }

    private string ReadString(string section, string key, string fallback) {
        var buffer = new StringBuilder(4096);
        var read = GetPrivateProfileString(section, key, fallback, buffer, buffer.Capacity, FilePath);
        return read == 0 ? fallback : buffer.ToString();
    }

    private bool WriteInt(string section, string key, int value) {
        return WriteString(section, key, value.ToString(CultureInfo.InvariantCulture));
    }

    private bool WriteString(string section, string? key, string? value) {
        return WritePrivateProfileString(section, key, value, FilePath);
    }

    [LibraryImport("kernel32.dll", StringMarshalling = StringMarshalling.Utf16, SetLastError = true)]
    private static partial int GetPrivateProfileInt(string lpAppName, string lpKeyName, int nDefault, string lpFileName);

    [LibraryImport("kernel32.dll", StringMarshalling = StringMarshalling.Utf16, SetLastError = true)]
    private static partial uint GetPrivateProfileString(
        string lpAppName,
        string lpKeyName,
        string lpDefault,
        StringBuilder lpReturnedString,
        int nSize,
        string lpFileName);

    [LibraryImport("kernel32.dll", StringMarshalling = StringMarshalling.Utf16, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool WritePrivateProfileString(string lpAppName, string? lpKeyName, string? lpString, string lpFileName);
}
