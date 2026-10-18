/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        X11CaptureBackend.h
 *  Description: X11 implementation of the capture backend
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

#include "CaptureBackend.h"

// Deliberately not including <X11/Xlib.h> here: its macros (Bool, True,
// False, Status, None, ...) collide with Qt's own identifiers if they leak
// into a translation unit that also includes Qt headers. Xlib.h is only
// included in the .cpp, after all Qt includes.
struct _XDisplay;
typedef struct _XDisplay Display;

class X11CaptureBackend : public CaptureBackend {
public:
    X11CaptureBackend();
    ~X11CaptureBackend() override;

    QImage grabRect(const QRect &globalRect) override;
    QRect virtualDesktopGeometry() const override;
    WindowInfo windowAt(const QPoint &globalPos,
                         const QVector<WindowHandle> &excluding) override;

private:
    QRect windowGeometry(unsigned long w) const;
    QString windowTitle(unsigned long w) const;
    bool isViewable(unsigned long w) const;

    Display *m_display = nullptr;
    unsigned long m_root = 0;
    unsigned long m_clientListStacking = 0;
    unsigned long m_wmName = 0;
    unsigned long m_utf8String = 0;
    unsigned long m_frameExtents = 0;
};
