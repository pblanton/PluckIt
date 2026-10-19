# CLAUDE.md

Guidance for Claude Code working in this repo.

## Read first

- `PLAN.md` -- current status, roadmap/TODOs, and the **hard constraint** that
  this project must stay a clean-room implementation (no reverse-engineering of
  any commercial app, ever -- see that file for why).
- `README.md` -- user-facing features, build/run instructions, architecture file
  map.

## Build & run

```
cmake -B build -S .
cmake --build build
./build/PluckIt
```

Dependencies (Debian/Ubuntu):
`qt6-base-dev libx11-dev libxext-dev libxfixes-dev cmake build-essential`. On
macOS (once the port in `PLAN.md` exists) this will differ -- check
`CMakeLists.txt` for the current platform branches.

There is no automated test suite yet. Verify UI changes by actually running the
app (`./build/PluckIt`) and exercising the feature -- for changes to
`EditorWindow`/`EditorCanvas` specifically, a quick way to sanity-check
rendering without simulating real mouse input is `QWidget::grab()` on the window
from a throwaway debug code path, saved to a PNG and inspected, then removed
before finishing -- don't leave debug hooks in `main.cpp`.

## Conventions

- C++17, Qt 6 Widgets. No comments except where a non-obvious *why* needs
  explaining (a subtle invariant, a workaround, something that would surprise a
  reader) -- not restating what the code does.
- `CaptureBackend` (`src/CaptureBackend.h`) is the platform abstraction seam.
  Don't let X11-specific types (`Display*`, `Window`, `Atom`) leak into
  cross-platform code; `X11CaptureBackend.h` deliberately avoids including
  `<X11/Xlib.h>` and uses `unsigned long` in its interface for exactly this
  reason -- Xlib's macros (`Bool`, `True`, `False`, `Status`, `None`, ...)
  collide with Qt identifiers if they leak into a translation unit that also
  includes Qt headers. If you add Xlib usage, include `<X11/Xlib.h>` **last**,
  after all Qt headers, in the `.cpp` only.
- New QSettings keys go in `src/SettingsKeys.h`, not inline string literals, so
  `CaptureSettings`/the settings UI and whatever reads the setting can't drift apart.
- Toolbar icons are procedurally drawn in `src/IconFactory.cpp`, not pulled from
  `QIcon::fromTheme()` -- icon theme availability/naming varies too much across
  GNOME/KDE/XFCE to rely on it. Follow the same pattern for new icons.
- Destructive image operations (crop, blur, pixelate) push an undo snapshot
  first (`EditorCanvas::pushUndoSnapshot`) and apply immediately -- no separate
  confirm step beyond what's already in the UI (e.g. crop's Apply/Enter). Keep
  that pattern for any new redaction/destructive tool.
