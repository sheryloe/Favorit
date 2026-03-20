# Favorit Wiki

## Project Summary

Favorite Widget is a compact Windows desktop launcher.
Favorites (app/file/url), position, and metadata are persisted in `favorite-widget.ini` under the local app data path.
The original Win32 launcher remains the runtime surface, while settings management is now handled by a dedicated WinUI 3 app.

## Quick Links

- [README.md](../README.md)
- [GitHub Pages](../docs/index.html)
- [Service-Roadmap.md](./Service-Roadmap.md)

## What is changed

- The settings window inside the main widget is now an execution path wrapper.
- If WinUI settings exe is available, it is launched directly.
- Widget auto-reloads when `favorite-widget.ini` changes.
- Legacy INI format and path are kept unchanged.

## Notes

- This keeps migration simple for existing users.
- CI/doc updates should be done together with UI behavior updates.
