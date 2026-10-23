# Changelog

All notable changes to PluckIt are recorded here, newest first.

## Unreleased

### Changed

- **One tabbed editor instead of a window per capture.** There is only ever one
  editor window. A new capture opens in a new tab and switches to it; if the
  editor isn't open yet, it opens with that one tab. Opening the editor yourself
  (the pencil, or Open Editor in the right-click menu) opens a single tab with
  the most recent capture, or switches to its tab if it's already open. The
  gear opens the settings page in the same window.
- Tabs are labeled with the capture time (the date too, for older captures);
  the tooltip shows the file name. Tabs close with their 'X' or Ctrl+W, and the
  window closes with its last tab. The toolbar's close button now closes the
  current tab rather than the whole window.
- Tool, color, and stroke width are shared across tabs; undo, crop, Save, and
  Copy apply to the current tab.
- Opening the most recent capture from the widget now reloads it from disk, so
  edits you saved earlier are shown rather than the original capture.
- New setting "Hide PluckIt when Capturing" hides all PluckIt windows when you create a new capture. 
  This is a setting you can turn off in the event you need to capture a PluckIt Window.

### Added

- **Copy from the editor**: a Copy button in the editor toolbar, and Ctrl+C
  (Cmd+C on macOS once ported), put the whole edited image -- annotations,
  redactions, and crop included -- on the clipboard, ready to paste into chat
  or messaging apps without saving first. A "Copied to clipboard" message
  confirms it.
- **Inter-document Hyperlinks added to all .md files**: I added inter-document
  hyperlinks to all .md files so that the reader can bonk on them to jump from
  document to document.

## A_0.02 -- 2026-10-19

### Added

- **Capture widget** replaces the old "pill". It docks to a screen edge; drag it
  toward another edge or monitor and it re-docks, rearranging its buttons
  between a horizontal strip (top/bottom) and a vertical one (left/right). The
  dock edge, monitor, and position are remembered between runs.
  - Round red **camera** button captures with the current mode. Click the small
    chevron on it (or right-click it) to choose Region, Window, Monitor (asks
    which display), or All Displays; the choice is remembered.
  - **Gear** opens a settings page inside the editor, **pencil** reopens the
    last capture in the editor, and the **hamburger** slides out the capture
    options tray.
  - Right-click menu: Open Editor / Settings / Exit.
- **Capture options** (in the tray and on the editor's settings page, kept in
  sync):
  - Open Capture in Editor (the old "open editor after capture" setting).
  - Copy to Clipboard.
  - Include Cursor (X11; needs `libxfixes-dev` at build time, otherwise the
    option is a no-op).
  - Capture Delay with a countdown shown on the camera button; click it again to
    cancel. Off by default, 5 seconds when enabled, remembered between runs.
  - Auto-hide at Edge.
- **Auto-hide**: when idle the widget collapses into a slim handle along its
  screen edge, as long as the widget itself, with a red grip line that stays on
  the widget when it is open. Resting the pointer on the handle for 350 ms
  slides it back out; moving away for 1 second tucks it away again. It stays
  open while dragging, while the tray or a menu is open, and during a capture
  countdown.
- **Settings page** in the editor window (gear icon in its toolbar). The editor
  can open on this page alone when there are no captures yet.
- `scripts/build-and-install.sh`: checks dependencies (offers to
  `apt-get install` missing ones), builds a Release binary, and installs it with
  a launcher entry and icons. Defaults to `~/.local`; options `--autostart`,
  `--system`, `--prefix DIR`, `--uninstall`, `--skip-deps`, `-y`.
- CMake `install()` rules, a `.desktop` launcher ([packaging/pluckit.desktop](packaging/pluckit.desktop)),
  and hicolor icons generated from the logo glyph.
- Application and window icon, and desktop-file name, set at startup.

### Changed

- The widget is hidden before every capture mode (including Monitor and All
  Displays) so it no longer appears in its own screenshots.
- Capture status ("Saved capture_....png" / "Capture failed") now appears as a
  brief tooltip instead of a label in the expanded panel.
- `CaptureBackend::grabRect` takes an `includeCursor` flag.
- The Configure dialog (`SettingsWindow`) is replaced by `SettingsPage`;
  settings keys moved/added in `SettingsKeys.h`. The old `pill/position` key is
  no longer used.
- Documentation re-wrapped to 80 columns.

### Fixed

- Region and window captures could include a cyan edge from the selection
  overlay's own outline. The overlays now hide and let the screen update before
  the grab happens.

### Known issues

- Include Cursor needs `libxfixes-dev` installed when building.
- Dragging a collapsed handle doesn't work; wait until the widget is expanded
before dragging it.
- Mixed-DPI multi-monitor setups and overlapped window captures are still
  imperfect (see "Known limitations" in [PLAN.md](PLAN.md)).

## A_0.01 -- 2026-10-18

- Initial Linux/X11 release: floating pill with Region, Window, Monitor, and All
  Displays capture; captures saved as timestamped PNGs under
  `~/Pictures/Captures`.
- Editor with arrow, rectangle, and ellipse annotations, crop with resize
  handles, pixelate and blur redaction, undo, and in-place save.
- Settings dialog with a single "open editor after capture" toggle.
