![if_life-pluckit-logo-400w.png](repo_files/PluckIt_Logo_PNGs/if_life-pluckit-logo-400w.png)

# PluckIt -- Plan / Roadmap

This file exists so a new contributor can get oriented fast. Read this, then
[README.md](README.md) for user-facing behavior. AI Assistants please read [CLAUDE.md](CLAUDE.md) for
working conventions.

## Hard constraint -- read this first

PluckIt is an **original, clean-room implementation**. It was explicitly built
*without* reverse-engineering any existing commercial screen-capture app. It
targets a generic "screen capture + annotate" feature set shared by many tools
in this space.

**This constraint must hold for all future work, including the macOS port
below.** Build the macOS backend against Apple's public screen-capture APIs
(ScreenCaptureKit / Core Graphics) and original UI/UX decisions -- never by
inspecting, decompiling, or imitating pixel-for-pixel any specific commercial
app's binary or internals.

Licensed GPLv3 (see [LICENSE](LICENSE)) specifically so forks/derivatives -- including a
macOS port -- stay open source rather than becoming someone's closed,
proprietary product.

## Current status

Current release: **A_0.02** (see [CHANGELOG.md](CHANGELOG.md)). Working end-to-end on
Linux/X11:
- Capture: region, window, per-monitor, all-displays. Multi-monitor aware.
- Capture widget docked to a screen edge (drag toward another edge or monitor to
  re-dock; buttons rearrange between horizontal and vertical), remembered
  position, camera button with a Region/Window/Monitor/All Displays mode menu,
  gear (settings page in the editor), pencil (open editor), and a slide-out
  options tray. Right-click menu (Open Editor / Settings / Exit).
- Editor: a single tabbed window; each capture opens in a new tab, and the
  pencil/gear open at most one tab (the most recent capture).
- Auto-hide: after a second without the pointer nearby the widget collapses into
  a slim handle along its screen edge (the red grip line stays visible when it
  is open); resting the pointer on the handle for a third of a second slides it
  back out. Toggle in the options.
- Capture options: hide PluckIt's own windows while capturing (on by default),
  preview in editor, copy to clipboard, include mouse cursor
  (needs `libxfixes-dev` at build time), and an optional countdown delay.
- Install: [scripts/build-and-install.sh](scripts/build-and-install.sh]) builds and installs
to `~/.local` with
  a launcher entry, icons, and optional start-at-login.
- Editor: arrow/rectangle/ellipse annotations, rich-text text blocks (any
  installed font, size, bold/italic/underline, per-run color; selectable,
  movable, editable), crop (with resize handles,
  Enter-to-apply), pixelate/blur redaction (destructive), undo, save (overwrites
  the capture file in place).
- Settings are stored with `QSettings` at `~/.config/PluckIt/PluckIt.conf`.

See [README.md's](README.md) architecture table for the file-by-file breakdown.

## TODO

- [x] Redesign pill into a multi-function floating widget.
- [ ] Design an option to run the application as a notification "tray" icon.
  When someone clicks it they will be displayed a small window that will provide
  them options for creating a new capture, changing settings, or quitting the
  application.
- [x] Allow multi-fuction floating widget to auto-hide when pinned to a monitor
  edge. Leave a handle with which to pull it back out.
- [x] Provide a setting to automatically add captures to the paste buffer.
- [ ] Allow users to select and copy/paste from the editor. (Copying the whole
  edited image works -- toolbar button and Ctrl+C. Still open: selecting a
  sub-region to copy, and pasting images into the editor.)
- [ ] Allow user to press [esc] to exit capture mode.
- [x] Provide a setting for capturing mouse cursor, or not.
- [x] Provide a setting for a capture timer. This will allow a user to start a
  capture, but have some pre-defined time to prepare the application for the
  screenshot. Sometimes you need to expand a context menu or otherwise interact
  with the application before you are ready to take the screenshot. The timer
  setting should include a delay time, and a checkbox to enable or disable the
  timer. Default to 5 seconds, but remember the user's delay setting between
  activations.
- [ ] Add Wayland support; notes below.
- [x] Create MacOS version; see `MacCaptureBackend.mm` / `MacWindowUtils.mm`.

### Wayland support -- notes

The current `X11CaptureBackend` talks to Xlib directly and won't work under a
Wayland session (`XDG_SESSION_TYPE=wayland` / `WAYLAND_DISPLAY` set). The
`CaptureBackend` interface (`src/headers/CaptureBackend.h`) already exists as the seam
for this -- add a `WaylandCaptureBackend` and select it in
`createCaptureBackend()` (`X11CaptureBackend.cpp`) based on session type at
startup.

Key differences to design around:
- Screenshots go through `xdg-desktop-portal`'s
  `org.freedesktop.portal.Screenshot` D-Bus interface (via `QtDBus`), not direct
  pixel access -- sandboxed by design, no raw framebuffer reads.
- Interactive region/window selection is typically delegated to the
  **compositor's own picker** when you request an "interactive" screenshot,
  rather than drawing our own overlay -- GNOME and KDE both restrict arbitrary
  full-screen input-grabbing overlays for security reasons. Our custom
  `RegionSelectOverlay`/`WindowSelectOverlay` may not be portable as-is; expect
  to special-case the Wayland path's UX.
- Continuous screen capture (if ever needed, e.g. for a future recording
  feature) goes through the `ScreenCast` portal + PipeWire, not a simple
  synchronous grab.
- `xdg-desktop-portal` and `xdg-desktop-portal-gnome`/`-gtk` are already present
  on the dev machine (confirmed during initial setup), so testing infrastructure
  exists.

### macOS port -- notes

The editor stack (`EditorWindow`, `EditorCanvas`, `Annotation`, `ImageEffects`,
`IconFactory`, `CaptureManager`, `SettingsWindow`, `SettingsKeys`) is plain Qt
Widgets/QtCore/QtGui with no X11 dependency -- it should need little to no
change. The capture layer is the real work:

1. **New backend**: implement `MacCaptureBackend` (likely Objective-C++, `.mm`)
   against `CaptureBackend`'s interface:
   - `grabRect` / `virtualDesktopGeometry`: `ScreenCaptureKit` (macOS 12.3+,
     preferred) or `CGDisplayCreateImage` / `CGWindowListCreateImage` for
     broader OS compatibility.
   - `windowAt`: `CGWindowListCopyWindowInfo` to enumerate on-screen windows and
     their geometry/ordering, replacing the `_NET_CLIENT_LIST_STACKING` logic in
     `X11CaptureBackend::windowAt`.
2. **Screen Recording permission**: unlike X11, macOS gates screen capture
   behind a user-granted Privacy & Security permission. The app needs to
   check/request it on first run and handle denial gracefully (X11 has nothing
   equivalent today -- this is new error-handling surface).
3. **Window flags**: `RegionSelectOverlay` and `WindowSelectOverlay` use
   `Qt::X11BypassWindowManagerHint`, which is a no-op outside X11. Verify
   `Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool` alone is
   sufficient for a borderless, input-grabbing, spans-all-displays overlay on
   macOS; may need platform-native `NSWindow` level tweaks via Qt's native
   handle APIs if not.
4. **Build system**: `CMakeLists.txt` currently unconditionally
   `find_package(X11 REQUIRED)` and compiles `X11CaptureBackend.cpp`. Add
   `if(APPLE)` / `else()` branches: link `ScreenCaptureKit`/`AppKit` frameworks
   and compile the Mac backend instead of the X11 one.
5. **Packaging**: `.app` bundling, code signing, notarization are unaddressed
   and out of scope until the backend itself works.

Suggested order: stub `MacCaptureBackend` returning blank images to get the app
compiling and the UI running on macOS first, then fill in
`virtualDesktopGeometry` (likely works via `QGuiApplication::screens()`
unchanged), then `grabRect`, then `windowAt`, then the permission flow last (so
you're not fighting permission prompts while the capture code itself is still
unreliable).

## Known limitations / nice-to-haves (not yet scheduled)

- Multi-monitor capture compositing (`X11CaptureBackend::grabRect`) assumes a
  **uniform device pixel ratio across monitors**; mixed-DPI rigs aren't handled
  correctly yet.
- Window capture grabs the window's on-screen visible pixels at its geometry,
  not an off-screen/composited buffer -- if another window is overlapping it at
  the moment of capture, that overlap will be included (no `XComposite`-based
  true window capture).
- No Redo (Undo only).
- No callout/stamp/highlighter annotation tools -- arrow/rectangle/ellipse,
  text, and the pixelate/blur redaction tools.
- Only text blocks can be selected, moved, and edited after placing; arrows and
  shapes still can't (no general Select tool).
- Text blocks have no background/outline/shadow or paragraph alignment yet;
  those would help readability over busy screenshots.
- No configurable save location/filename pattern yet.
- No global hotkey to trigger a capture -- the widget must be clicked.
- A collapsed (auto-hidden) handle can't be dragged directly; the first press
  expands it.
- Autostart-on-login is only available through the install script
  (`--autostart`); there is no in-app toggle.
