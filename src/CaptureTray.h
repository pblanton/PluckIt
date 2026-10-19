/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureTray.h
 *  Description: Slide-out tray window holding the capture options
 *
 *  Author:      Phillip H. Blanton
 *  Created:     Oct 19, 2026
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

class QVariantAnimation;

enum class DockEdge { Left, Right, Top, Bottom };

// Frameless window that slides out of the capture widget, away from the screen
// edge the widget is docked to. Kept as its own top-level window rather than
// growing the widget itself so the widget's position and layout don't shift
// while it opens.
class CaptureTray : public QWidget {
    Q_OBJECT
public:
    explicit CaptureTray(QWidget *parent = nullptr);

    // anchor is the widget's geometry; bounds is the work area of its screen,
    // used to keep the tray on-screen along the dock edge.
    void slideOut(const QRect &anchor, DockEdge edge, const QRect &bounds);
    void slideIn();
    void closeNow();
    bool isOpen() const { return m_open; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void applyProgress(qreal t);

    QWidget *m_content = nullptr;
    QVariantAnimation *m_anim = nullptr;
    QRect m_anchor;
    DockEdge m_edge = DockEdge::Right;
    QPoint m_origin;
    qreal m_progress = 0;
    bool m_open = false;
};
