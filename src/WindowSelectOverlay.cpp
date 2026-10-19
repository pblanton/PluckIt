/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        WindowSelectOverlay.cpp
 *  Description: Window-highlighting overlay painting and click handling
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

#include "WindowSelectOverlay.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

namespace {
constexpr int kUnmapSettleMs = 120;
}

WindowSelectOverlay::WindowSelectOverlay(CaptureBackend *backend, const QRect &virtualDesktopRect,
                                          QWidget *parent)
    : QWidget(parent)
    , m_backend(backend)
    , m_virtualRect(virtualDesktopRect)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool
                   | Qt::X11BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setGeometry(m_virtualRect);
}

void WindowSelectOverlay::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    raise();
    activateWindow();
    setFocus(Qt::OtherFocusReason);
}

void WindowSelectOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 70));

    if (m_current.handle != 0) {
        const QRect local = m_current.geometry.translated(-m_virtualRect.topLeft());

        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(local, Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.setPen(QPen(QColor(0, 174, 255), 3));
        painter.drawRect(local.adjusted(1, 1, -2, -2));

        if (!m_current.title.isEmpty()) {
            painter.setPen(Qt::white);
            painter.drawText(local.topLeft() + QPoint(6, 18), m_current.title);
        }
    }
}

void WindowSelectOverlay::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint globalPos = event->pos() + m_virtualRect.topLeft();
    const WindowInfo info = m_backend->windowAt(globalPos, {WindowHandle(winId())});
    if (info.handle != m_current.handle) {
        m_current = info;
        update();
    }
}

void WindowSelectOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (m_current.handle != 0) {
        const WindowInfo selected = m_current;
        // The receiver grabs the screen from this signal, so the overlay (and
        // its outline) has to be gone from the screen first.
        hide();
        QTimer::singleShot(kUnmapSettleMs, this, [this, selected]() {
            emit windowSelected(selected);
            close();
        });
        return;
    }
    emit cancelled();
    close();
}

void WindowSelectOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancelled();
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}
