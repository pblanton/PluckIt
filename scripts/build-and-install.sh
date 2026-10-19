#!/usr/bin/env bash
# Build PluckIt in Release mode and install it (binary, launcher entry, icons).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build-release"
PREFIX="$HOME/.local"
SYSTEM=0
AUTOSTART=0
UNINSTALL=0
SKIP_DEPS=0
ASSUME_YES=0

DEB_PACKAGES=(qt6-base-dev libx11-dev libxext-dev libxfixes-dev libxkbcommon-dev cmake build-essential)
AUTOSTART_FILE="${XDG_CONFIG_HOME:-$HOME/.config}/autostart/pluckit.desktop"

usage() {
    cat <<USAGE
Usage: scripts/build-and-install.sh [options]

  --prefix DIR    Install under DIR (default: ~/.local)
  --system        Install to /usr/local for all users (uses sudo)
  --autostart     Also start PluckIt automatically at login
  --uninstall     Remove a previous install (honours --prefix/--system)
  --skip-deps     Don't check for or install build dependencies
  -y, --yes       Don't ask before installing missing packages
  -h, --help      Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix) PREFIX="${2:?--prefix needs a directory}"; shift ;;
        --system) SYSTEM=1; PREFIX=/usr/local ;;
        --autostart) AUTOSTART=1 ;;
        --uninstall) UNINSTALL=1 ;;
        --skip-deps) SKIP_DEPS=1 ;;
        -y|--yes) ASSUME_YES=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
    esac
    shift
done

say() { printf '\n==> %s\n' "$*"; }

SUDO=()
if [[ $SYSTEM -eq 1 && $EUID -ne 0 ]]; then
    SUDO=(sudo)
fi

stop_running() {
    if pgrep -x PluckIt >/dev/null; then
        say "Stopping the running PluckIt"
        pkill -x PluckIt || true
        sleep 1
    fi
}

if [[ $UNINSTALL -eq 1 ]]; then
    say "Uninstalling PluckIt from $PREFIX"
    stop_running
    "${SUDO[@]}" rm -f "$PREFIX/bin/PluckIt" "$PREFIX/share/applications/pluckit.desktop"
    for size in 16 24 32 48 64 96 128 256 512; do
        "${SUDO[@]}" rm -f "$PREFIX/share/icons/hicolor/${size}x${size}/apps/pluckit.png"
    done
    rm -f "$AUTOSTART_FILE"
    echo "Done. Captures in ~/Pictures/Captures and settings in ~/.config/PluckIt were left alone."
    exit 0
fi

check_deps() {
    if ! command -v dpkg >/dev/null || ! command -v apt-get >/dev/null; then
        echo "Not a Debian/Ubuntu system: make sure Qt 6 Widgets/Network, Xlib, Xext, XFixes (dev packages)," >&2
        echo "CMake >= 3.16 and a C++17 compiler are installed." >&2
        return
    fi

    local missing=()
    for pkg in "${DEB_PACKAGES[@]}"; do
        dpkg -s "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
    done
    [[ ${#missing[@]} -eq 0 ]] && return

    say "Missing packages: ${missing[*]}"
    if [[ $ASSUME_YES -ne 1 ]]; then
        read -r -p "Install them with sudo apt-get? [Y/n] " reply
        if [[ "$reply" =~ ^[Nn] ]]; then
            echo "Continuing without them; the build may fail" \
                 "(without libxfixes-dev it works but can't draw the mouse cursor into captures)." >&2
            return
        fi
    fi
    sudo apt-get update
    sudo apt-get install -y "${missing[@]}"
}

[[ $SKIP_DEPS -eq 1 ]] || check_deps

say "Configuring (Release, prefix $PREFIX)"
cmake -B "$BUILD_DIR" -S "$ROOT" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX"

say "Building"
cmake --build "$BUILD_DIR" -j"$(nproc)"

stop_running

say "Installing"
"${SUDO[@]}" cmake --install "$BUILD_DIR"

if command -v gtk-update-icon-cache >/dev/null && [[ -f "$PREFIX/share/icons/hicolor/index.theme" ]]; then
    "${SUDO[@]}" gtk-update-icon-cache -q -f -t "$PREFIX/share/icons/hicolor" || true
fi
if command -v update-desktop-database >/dev/null; then
    "${SUDO[@]}" update-desktop-database -q "$PREFIX/share/applications" || true
fi

if [[ $AUTOSTART -eq 1 ]]; then
    say "Enabling start at login"
    mkdir -p "$(dirname "$AUTOSTART_FILE")"
    {
        sed "s|^Exec=.*|Exec=$PREFIX/bin/PluckIt|" "$ROOT/packaging/pluckit.desktop"
        echo "X-GNOME-Autostart-enabled=true"
    } > "$AUTOSTART_FILE"
fi

say "Installed to $PREFIX/bin/PluckIt"
case ":$PATH:" in
    *":$PREFIX/bin:"*) ;;
    *) echo "Note: $PREFIX/bin isn't on your PATH; launch it from the app menu or run $PREFIX/bin/PluckIt." ;;
esac
echo "Start it from the app menu (search for PluckIt)."
[[ $AUTOSTART -eq 1 ]] || echo "Re-run with --autostart to have it start at login."
