![if_life-pluckit-logo-1000w.png](repo_files/PluckIt_Logo_PNGs/if_life-pluckit-logo-1000w.png)

## This is a work in progress!!

Current version: **A_0.02** -- see `CHANGELOG.md` for what changed.

PluckIt is an original, Linux-first screen capture and annotation tool. It runs
as a small always-on-top capture widget, captures a region/window/monitor/
all-displays, and opens an editor for arrows, shapes, crop, blur, and pixelate
before you share the result.

It's a clean-room implementation written from scratch for this project. It is
not a decompiled, reverse-engineered, or ported clone of any existing commercial
application -- it was designed around a generic "screen capture + annotate"
feature set that's common to many tools in this space (Snagit, Flameshot,
Greenshot, KSnip, ShareX, etc). See [PLAN.md](PLAN.md) for why this is important
and must remain the focus moving forward.

## Features

- **Capture modes**: click-drag region, window picker (hover-highlight, click to
  grab), per-monitor, or the full virtual desktop across all monitors.
  Multi-monitor aware, including mixed monitor layouts.
- **Capture widget**: always-on-top and docked to a screen edge. Drag it toward
  another edge or monitor and it re-docks, switching between a horizontal and a
  vertical layout; it remembers where you left it between runs. A red camera
  button captures, a gear opens settings, a pencil reopens the editor, and a
  hamburger slides out the capture options.
- **Auto-hide**: when idle the widget slides into its screen edge, leaving a
  small handle. Rest the pointer on the handle briefly (about a third of a
  second) and it slides back out; moving the pointer away for about a second
  tucks it away again. Can be turned off in the options.
- **Capture options**: open capture in editor, copy to clipboard, include the
  mouse cursor, auto-hide, and an optional countdown delay before capturing.
- **Editor**: opens on every capture (unless "Open Capture in Editor" is turned
  off).
  - Arrow, Rectangle, Ellipse annotations with adjustable color and stroke
    width.
  - **Pixelate** and **Blur** for redacting sensitive info (passwords, etc).
    These are destructive (baked into the image immediately), with Undo as the
    safety net.
  - **Crop** with draggable resize handles on all 8 corners/edges; Enter applies
    it, or use the Apply Crop button.
  - Undo (stack-based, covers annotations, redactions, and crop).
  - Save overwrites the already-saved capture file in place.
- **Settings**: the same capture options, available from the tray or from the
  gear (a settings page inside the editor window). More to come -- see
  `PLAN.md`.

## Requirements

- Linux, X11 session (see `PLAN.md` for Wayland status)
- Qt 6 (Widgets, Network)
- X11 development libraries (Xlib, Xext, and XFixes for cursor capture)
- CMake >= 3.16, a C++17 compiler

On Debian/Ubuntu-based distros:

```
sudo apt install -y qt6-base-dev libx11-dev libxext-dev libxfixes-dev \
  cmake build-essential
```

## Build

```
cmake -B build -S .
cmake --build build
```

## Install

On Debian/Ubuntu-based systems (including Zorin) one script checks the
dependencies, builds a Release binary, and installs it with a launcher entry and
icons:

```
scripts/build-and-install.sh
```

It installs under `~/.local` by default, so no root is needed except for
installing missing packages. Options:

- `--autostart` -- also start PluckIt at login.
- `--system` -- install to `/usr/local` for all users (uses sudo).
- `--prefix DIR` -- install somewhere else.
- `--uninstall` -- remove it again (captures and settings are kept).
- `--skip-deps`, `-y` -- skip the dependency check, or don't ask before
  installing packages.

Re-running the script upgrades an existing install (it stops a running PluckIt
first). Afterwards, launch PluckIt from the app menu.

## Run

```
./build/PluckIt
```

This starts the capture widget (single-instance -- launching it again while one
is already running is a no-op). It keeps running in the background; there's no
"main window" to close. To quit, right-click the widget and choose **Exit**.

## Usage

- **Click the camera** to capture using the current mode. Click the small
  chevron on the camera (or right-click it) to pick the mode: **Region**,
  **Window**, **Monitor** (asks which display), or **All Displays**. The choice
  is remembered.
- **Pencil** -- reopen the editor on your most recent capture (disabled until
  you've captured at least once).
- **Gear** -- open the settings page in the editor.
- **Hamburger** -- slide the capture options tray in or out.
- **Drag** the widget to reposition it. It sticks to the nearest screen edge;
  the new position is remembered across restarts.
- **Right-click** the widget for Open Editor / Settings / **Exit**.
- With **Capture Delay** enabled, the camera counts down before capturing so you
  can set up the shot; click it again to cancel.
- Every capture is saved immediately to `~/Pictures/Captures/` as
  `capture_<timestamp>.png`, regardless of whether you edit it afterward.
- In the editor, **Esc** cancels an in-progress crop/region selection; **Enter**
  applies a pending crop.

## Where things live

- Captures: `~/Pictures/Captures/`
- Settings: `~/.config/PluckIt/PluckIt.conf` (plain Qt INI file; widget dock
  position and the capture options live here)

## Architecture (quick map)

| File | Responsibility |
|---|---|
| `scripts/build-and-install.sh` / `packaging/` | Build-and-install script, launcher entry (`.desktop`) |
| `main.cpp` | App entry point, single-instance guard, org/app name setup for settings |
| `FloatingWidget` | The capture widget: edge docking, button layout, capture flow (mode, delay, clipboard), right-click menu, position persistence |
| `CaptureButton` | The round camera button: mode-menu chevron and countdown display |
| `CaptureTray` | The slide-out window that hosts the capture options |
| `CaptureOptionsPanel` / `ToggleSwitch` | The option rows (shared by the tray and the settings page) and the switch control |
| `CaptureSettings` | Observable, QSettings-backed capture options and current mode |
| `CaptureBackend` / `X11CaptureBackend` | Abstract capture interface + the only implementation today (X11, Xlib) |
| `RegionSelectOverlay` | Click-drag bounding-box selection overlay |
| `WindowSelectOverlay` | Hover-highlight-a-window selection overlay |
| `CaptureManager` | Saves a QImage to a timestamped PNG under `~/Pictures/Captures` |
| `EditorWindow` | The editor's `QMainWindow`: toolbars, actions, wiring |
| `EditorCanvas` | The editor's drawing surface: tool state, mouse handling, crop handles |
| `Annotation` | A single vector shape (arrow/rectangle/ellipse) and how to draw it |
| `ImageEffects` | Pixelate/blur pixel manipulation, applied destructively to a region |
| `IconFactory` | Procedurally-drawn toolbar icons (no dependency on a system icon theme) |
| `SettingsPage` / `SettingsKeys` | The editor's settings page and the QSettings key names |

See `PLAN.md` for the current roadmap, known limitations, and porting notes
(Wayland, macOS).

## License

GPLv3 -- see `LICENSE`. Forks and modifications are welcome, but any distributed
version (modified or not) must stay open source under GPLv3.

