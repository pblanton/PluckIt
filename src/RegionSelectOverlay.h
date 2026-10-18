/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        RegionSelectOverlay.h
 *  Description: Fullscreen click-drag region selection overlay
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

#include <QRect>
#include <QWidget>

// Fullscreen (spanning the whole virtual desktop) click-drag rectangle
// selector. Emits regionSelected() with a rect in global/root coordinates.
class RegionSelectOverlay : public QWidget {
    Q_OBJECT
public:
    explicit RegionSelectOverlay(const QRect &virtualDesktopRect, QWidget *parent = nullptr);

signals:
    void regionSelected(const QRect &globalRect);
    void cancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QRect m_virtualRect;
    QPoint m_origin;
    QRect m_selection;
    bool m_selecting = false;
};
