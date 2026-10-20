/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureButton.cpp
 *  Description: Painting and click routing for the round capture button
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

#include "../headers/CaptureButton.h"

#include "../headers/IconFactory.h"

#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

CaptureButton::CaptureButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
}

QSize CaptureButton::sizeHint() const
{
    return QSize(64, 64);
}

void CaptureButton::setCountdown(int seconds)
{
    m_countdown = seconds;
    setToolTip(seconds > 0 ? QStringLiteral("Click to cancel the countdown") : QString());
    update();
}

bool CaptureButton::hitButton(const QPoint &pos) const
{
    const QPointF c = QRectF(rect()).center();
    const qreal r = qMin(width(), height()) / 2.0;
    const QPointF d = QPointF(pos) - c;
    return d.x() * d.x() + d.y() * d.y() <= r * r;
}

bool CaptureButton::inChevronZone(const QPoint &pos) const
{
    return m_countdown == 0 && hitButton(pos) && pos.x() > width() * 0.6 && pos.y() > height() * 0.6;
}

void CaptureButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor fill(229, 57, 53);
    if (isDown()) {
        fill = fill.darker(120);
    } else if (underMouse()) {
        fill = fill.lighter(112);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawEllipse(QRectF(rect()).adjusted(1, 1, -1, -1));

    if (m_countdown > 0) {
        QFont f = font();
        f.setBold(true);
        f.setPixelSize(height() * 0.45);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, QString::number(m_countdown));
        return;
    }

    const QRectF glyph = QRectF(rect()).adjusted(width() * 0.2, height() * 0.2, -width() * 0.2, -height() * 0.2);
    IconFactory::paintGlyph(p, IconFactory::Glyph::Camera, glyph, Qt::white);

    const QPointF chev(width() * 0.77, height() * 0.8);
    const qreal w = width() * 0.07;
    QPainterPath path;
    path.moveTo(chev + QPointF(-w, -w * 0.5));
    path.lineTo(chev + QPointF(0, w * 0.5));
    path.lineTo(chev + QPointF(w, -w * 0.5));
    p.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}

void CaptureButton::mousePressEvent(QMouseEvent *event)
{
    m_chevronPressed = event->button() == Qt::LeftButton && inChevronZone(event->position().toPoint());
    if (m_chevronPressed) {
        event->accept();
        return;
    }
    QAbstractButton::mousePressEvent(event);
}

void CaptureButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_chevronPressed) {
        m_chevronPressed = false;
        if (inChevronZone(event->position().toPoint())) {
            emit modeMenuRequested();
        }
        event->accept();
        return;
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void CaptureButton::contextMenuEvent(QContextMenuEvent *event)
{
    emit modeMenuRequested();
    event->accept();
}

void CaptureButton::enterEvent(QEnterEvent *event)
{
    update();
    QAbstractButton::enterEvent(event);
}

void CaptureButton::leaveEvent(QEvent *event)
{
    update();
    QAbstractButton::leaveEvent(event);
}
