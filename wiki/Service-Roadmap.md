# Favorit Service Roadmap

## Scope

The roadmap focuses on finishing the WinUI 3 settings split, packaging stabilization, and practical operations hardening.

## Priority 1

- Settings UI is now WinUI 3 with list + form + command bar.
- INI read/write service in WinUI uses the exact legacy path and sections.
- Automatic widget refresh works when `favorite-widget.ini` changes.
- GitHub Pages and `README.md` reflect the WinUI settings architecture and paths.

## Priority 2

- Add CI check for WinUI build environment preconditions.
- Add optional automated settings snapshot test (INI round-trip and schema).
- Improve startup reliability and crash-safe handling around settings launch path resolution.

## Ops

- Add explicit release checklist for both widget and settings packages.
- Publish MSIX settings installer artifact alongside NSIS and ZIP.
- Expand user troubleshooting documentation for environments where WinUI toolchain is unavailable.
