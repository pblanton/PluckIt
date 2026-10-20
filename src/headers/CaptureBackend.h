/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureBackend.h
 *  Description: Platform abstraction interface for screen capture backends
 *
 *  Author:      Phillip H. Blanton
 *  Created:     Oct 18, 2026
 *
 *  Copyright (c) 2026 Phillip H. Blanton
 *
 *  SPDX-License-Identifier: GPLv3
 *
 *  PluckIt is free and open-source software.
 *  See the LICENSE file in the project root for license details.
 *
 *  "If you can see it, then you can pluck it."
 * ============================================================================
 */

#pragma once

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>
#include <memory>

using WindowHandle = unsigned long; // X11 Window id

struct WindowInfo {
    WindowHandle handle = 0;
    QRect geometry;       // global/root coordinates
    QString title;
};

// Abstraction over the display-server-specific capture mechanism.
// X11CaptureBackend is the first implementation; a Wayland backend
// (xdg-desktop-portal Screenshot/ScreenCast) can implement this later.
class CaptureBackend {
public:
    virtual ~CaptureBackend() = default;

    // Grabs the given rectangle in global/root (virtual desktop) coordinates.
    // May span multiple physical monitors. Backends that can't draw the
    // pointer ignore includeCursor.
    virtual QImage grabRect(const QRect &globalRect, bool includeCursor = false) = 0;

    // Bounding rectangle of the full virtual desktop (union of all monitors).
    virtual QRect virtualDesktopGeometry() const = 0;

    // Finds the top-level window at a given global point, excluding any
    // window handle listed in `excluding` (used to skip our own overlays).
    virtual WindowInfo windowAt(const QPoint &globalPos,
                                 const QVector<WindowHandle> &excluding) = 0;
};

std::unique_ptr<CaptureBackend> createCaptureBackend();
