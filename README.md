# If you can see it, PluckIt!

## This is a work in progress!! 

PluckIt is an original, Linux-first screen capture and annotation tool. It runs as a small always-on-top floating pill, 
captures a region/window/monitor/ all-displays, and opens an editor for arrows, shapes, crop, blur, and pixelate before 
you share the result.

It's a clean-room implementation written from scratch for this project. It is not a decompiled, reverse-engineered, or
ported clone of any existing commercial application -- it was designed around a generic "screen capture + annotate" 
feature set that's common to many tools in this space (Snagit, Flameshot, Greenshot, KSnip, ShareX, etc). See 
`PLAN.md` for why that distinction matters and must stay true going forward.

## Features

- **Capture modes**: click-drag region, window picker (hover-highlight, click to grab), per-monitor, or the full virtual
- desktop across all monitors. Multi-monitor aware, including mixed monitor layouts.
- **Floating pill UI**: always-on-top, draggable, remembers where you left it between runs. Click to expand the
- capture-mode panel; right-click for Show Editor / Configure / Exit.
- **Editor**: opens on every capture (unless turned off in Configure).
  - Arrow, Rectangle, Ellipse annotations with adjustable color and stroke width.
  - **Pixelate** and **Blur** for redacting sensitive info (passwords, etc).
    These are destructive (baked into the image immediately), with Undo as the safety net.
  - **Crop** with draggable resize handles on all 8 corners/edges; Enter applies it, or use the Apply Crop button.
  - Undo (stack-based, covers annotations, redactions, and crop).
  - Save overwrites the already-saved capture file in place.
- **Settings** (right-click pill -> Configure): currently just whether the editor opens automatically after a capture.
More to come -- see `PLAN.md`.

## Requirements

- Linux, X11 session (see `PLAN.md` for Wayland status)
- Qt 6 (Widgets, Network)
- X11 development libraries (Xlib, Xext)
- CMake >= 3.16, a C++17 compiler

On Debian/Ubuntu-based distros:

```
sudo apt install -y qt6-base-dev libx11-dev libxext-dev cmake build-essential
```

## Build

```
cmake -B build -S .
cmake --build build
```

## Run

```
./build/PluckIt
```

This starts the floating pill (single-instance -- launching it again while one is already running is a no-op). It keeps 
running in the background; there's no "main window" to close. To quit, right-click the pill and choose **Exit**.

## Usage

- **Click** the pill to expand/collapse the capture-mode panel.
- **Drag** the pill to reposition it -- the new position is remembered across restarts.
- **Right-click** the pill for:
  - **Show Editor** -- reopen the editor on your most recent capture (disabled until you've captured at least once this
  session).
  - **Configure** -- open settings.
  - **Exit** -- quit PluckIt.
- Capture modes in the expanded panel: **Region**, **Window**, **Monitor** (submenu per connected display), 
**All Displays**.
- Every capture is saved immediately to `~/Pictures/Captures/` as `capture_<timestamp>.png`, regardless of whether you 
edit it afterward.
- In the editor, **Esc** cancels an in-progress crop/region selection;
  **Enter** applies a pending crop.

## Where things live

- Captures: `~/Pictures/Captures/`
- Settings: `~/.config/PluckIt/PluckIt.conf` (plain Qt INI file; pill
  position and the editor-after-capture toggle live here)

## Architecture (quick map)

| File | Responsibility |
|---|---|
| `main.cpp` | App entry point, single-instance guard, org/app name setup for settings |
| `FloatingWidget` | The pill UI: drag, expand panel, capture-mode buttons, right-click menu, position persistence |
| `CaptureBackend` / `X11CaptureBackend` | Abstract capture interface + the only implementation today (X11, Xlib) |
| `RegionSelectOverlay` | Click-drag bounding-box selection overlay |
| `WindowSelectOverlay` | Hover-highlight-a-window selection overlay |
| `CaptureManager` | Saves a QImage to a timestamped PNG under `~/Pictures/Captures` |
| `EditorWindow` | The editor's `QMainWindow`: toolbars, actions, wiring |
| `EditorCanvas` | The editor's drawing surface: tool state, mouse handling, crop handles |
| `Annotation` | A single vector shape (arrow/rectangle/ellipse) and how to draw it |
| `ImageEffects` | Pixelate/blur pixel manipulation, applied destructively to a region |
| `IconFactory` | Procedurally-drawn toolbar icons (no dependency on a system icon theme) |
| `SettingsWindow` / `SettingsKeys` | The Configure dialog and its QSettings key names |

See `PLAN.md` for the current roadmap, known limitations, and porting notes
(Wayland, macOS).

## License

GPLv3 -- see `LICENSE`. Forks and modifications are welcome, but any
distributed version (modified or not) must stay open source under GPLv3.

