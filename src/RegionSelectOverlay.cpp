/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        RegionSelectOverlay.cpp
 *  Description: Region selection overlay painting and mouse/keyboard handling
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

#include "RegionSelectOverlay.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

namespace {
constexpr int kUnmapSettleMs = 120;
}

RegionSelectOverlay::RegionSelectOverlay(const QRect &virtualDesktopRect, QWidget *parent)
    : QWidget(parent)
    , m_virtualRect(virtualDesktopRect)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool
                   | Qt::X11BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
    setGeometry(m_virtualRect);
}

void RegionSelectOverlay::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    raise();
    activateWindow();
    setFocus(Qt::OtherFocusReason);
}

void RegionSelectOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 100));

    if (!m_selection.isNull()) {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(m_selection, Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.setPen(QPen(QColor(0, 174, 255), 2));
        painter.drawRect(m_selection.adjusted(0, 0, -1, -1));

        const QString dims = QStringLiteral("%1 x %2").arg(m_selection.width()).arg(m_selection.height());
        painter.setPen(Qt::white);
        painter.drawText(m_selection.bottomLeft() + QPoint(4, 16), dims);
    }
}

void RegionSelectOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    m_origin = event->pos();
    m_selection = QRect(m_origin, QSize());
    m_selecting = true;
    update();
}

void RegionSelectOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_selecting) {
        return;
    }
    m_selection = QRect(m_origin, event->pos()).normalized();
    update();
}

void RegionSelectOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_selecting) {
        return;
    }
    m_selecting = false;

    if (m_selection.width() > 2 && m_selection.height() > 2) {
        const QRect globalRect = m_selection.translated(m_virtualRect.topLeft());
        // The receiver grabs the screen from this signal, so the overlay (and
        // its outline) has to be gone from the screen first.
        hide();
        QTimer::singleShot(kUnmapSettleMs, this, [this, globalRect]() {
            emit regionSelected(globalRect);
            close();
        });
        return;
    }
    emit cancelled();
    close();
}

void RegionSelectOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancelled();
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}
