/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        WindowSelectOverlay.h
 *  Description: Fullscreen overlay for picking a window to capture
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

#include <QRect>
#include <QWidget>

// Fullscreen overlay that highlights whichever top-level window is under the
// cursor (queried from the backend, not from Qt's own widget tree, since the
// target windows belong to other applications) and emits windowSelected()
// when the user clicks it.
class WindowSelectOverlay : public QWidget {
    Q_OBJECT
public:
    WindowSelectOverlay(CaptureBackend *backend, const QRect &virtualDesktopRect, QWidget *parent = nullptr);

signals:
    void windowSelected(const WindowInfo &info);
    void cancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    CaptureBackend *m_backend;
    QRect m_virtualRect;
    WindowInfo m_current;
};
