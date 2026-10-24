/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation
 * ============================================================================
 *
 *  File:        MacCaptureBackend.h
 *  Description: macOS implementation of the capture backend
 *
 *  Author:      Phillip H. Blanton
 *  Created:     Oct 23, 2026
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

#include "CaptureBackend.h"

// No CoreGraphics/AppKit headers here -- ObjC types must not leak into
// translation units that also include Qt headers. All Apple API calls live
// in MacCaptureBackend.mm.
class MacCaptureBackend : public CaptureBackend {
public:
    MacCaptureBackend();
    ~MacCaptureBackend() override;

    QImage grabRect(const QRect &globalRect, bool includeCursor = false) override;
    QRect virtualDesktopGeometry() const override;
    WindowInfo windowAt(const QPoint &globalPos,
                        const QVector<WindowHandle> &excluding) override;

    // Returns false if the user has denied Screen Recording in System Settings.
    static bool hasScreenRecordingPermission();
};
