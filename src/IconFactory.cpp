/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        IconFactory.cpp
 *  Description: Painter-based implementations of the toolbar icons
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

#include "IconFactory.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace IconFactory {

namespace {

const QColor kStroke(60, 60, 60);

QPixmap blankPixmap(int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    return pm;
}

void drawArrow(QPainter &p, qreal size)
{
    const qreal m = size * 0.18;
    p.setPen(QPen(kStroke, size * 0.1, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(m, size - m), QPointF(size - m, m));

    QPolygonF head;
    head << QPointF(size - m, m) << QPointF(size - m * 2.2, m) << QPointF(size - m, m * 2.2);
    p.setBrush(kStroke);
    p.setPen(Qt::NoPen);
    p.drawPolygon(head);
}

void drawRectangle(QPainter &p, qreal size)
{
    const qreal m = size * 0.2;
    p.setPen(QPen(kStroke, size * 0.1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(m, m, size - 2 * m, size - 2 * m));
}

void drawEllipse(QPainter &p, qreal size)
{
    const qreal m = size * 0.18;
    p.setPen(QPen(kStroke, size * 0.1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(m, m, size - 2 * m, size - 2 * m));
}

void drawPixelate(QPainter &p, qreal size)
{
    const int cells = 3;
    const qreal m = size * 0.15;
    const qreal cell = (size - 2 * m) / cells;
    const QColor shades[cells][cells] = {
        {QColor(90, 90, 90), QColor(150, 150, 150), QColor(110, 110, 110)},
        {QColor(160, 160, 160), QColor(80, 80, 80), QColor(170, 170, 170)},
        {QColor(100, 100, 100), QColor(140, 140, 140), QColor(70, 70, 70)},
    };
    p.setPen(Qt::NoPen);
    for (int row = 0; row < cells; ++row) {
        for (int col = 0; col < cells; ++col) {
            p.setBrush(shades[row][col]);
            p.drawRect(QRectF(m + col * cell, m + row * cell, cell - 1, cell - 1));
        }
    }
}

void drawBlur(QPainter &p, qreal size)
{
    const QPointF center(size / 2, size / 2);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(kStroke.red(), kStroke.green(), kStroke.blue(), 60));
    p.drawEllipse(center, size * 0.42, size * 0.42);
    p.setBrush(QColor(kStroke.red(), kStroke.green(), kStroke.blue(), 110));
    p.drawEllipse(center, size * 0.3, size * 0.3);
    p.setBrush(QColor(kStroke.red(), kStroke.green(), kStroke.blue(), 200));
    p.drawEllipse(center, size * 0.16, size * 0.16);
}

void drawCrop(QPainter &p, qreal size)
{
    const qreal m = size * 0.2;
    const qreal armLen = size * 0.28;
    p.setPen(QPen(kStroke, size * 0.1, Qt::SolidLine, Qt::SquareCap));

    // top-left corner bracket
    p.drawLine(QPointF(m, m + armLen), QPointF(m, m));
    p.drawLine(QPointF(m, m), QPointF(m + armLen, m));

    // bottom-right corner bracket
    p.drawLine(QPointF(size - m, size - m - armLen), QPointF(size - m, size - m));
    p.drawLine(QPointF(size - m, size - m), QPointF(size - m - armLen, size - m));
}

void drawUndo(QPainter &p, qreal size)
{
    const QRectF arcRect(size * 0.2, size * 0.2, size * 0.6, size * 0.6);
    p.setPen(QPen(kStroke, size * 0.1, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect, 20 * 16, 280 * 16);

    const QPointF tip(size * 0.26, size * 0.32);
    QPolygonF head;
    head << tip << QPointF(tip.x() + size * 0.2, tip.y() - size * 0.04)
         << QPointF(tip.x() + size * 0.02, tip.y() + size * 0.18);
    p.setBrush(kStroke);
    p.setPen(Qt::NoPen);
    p.drawPolygon(head);
}

void drawSave(QPainter &p, qreal size)
{
    const qreal m = size * 0.18;
    QPainterPath body;
    body.addRoundedRect(QRectF(m, m, size - 2 * m, size - 2 * m), size * 0.06, size * 0.06);
    p.setPen(QPen(kStroke, size * 0.08));
    p.setBrush(Qt::NoBrush);
    p.drawPath(body);

    // "label" slot near the bottom
    p.drawRect(QRectF(m + size * 0.12, size - m - size * 0.26, size - 2 * m - size * 0.24, size * 0.2));
    // metal slider near the top
    p.fillRect(QRectF(m + size * 0.18, m, size * 0.3, size * 0.16), kStroke);
}

void drawClose(QPainter &p, qreal size)
{
    const qreal m = size * 0.26;
    p.setPen(QPen(kStroke, size * 0.1, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(m, m), QPointF(size - m, size - m));
    p.drawLine(QPointF(size - m, m), QPointF(m, size - m));
}

} // namespace

QIcon icon(Kind kind, int size)
{
    QPixmap pm = blankPixmap(size);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    switch (kind) {
    case Kind::Arrow: drawArrow(p, size); break;
    case Kind::Rectangle: drawRectangle(p, size); break;
    case Kind::Ellipse: drawEllipse(p, size); break;
    case Kind::Pixelate: drawPixelate(p, size); break;
    case Kind::Blur: drawBlur(p, size); break;
    case Kind::Crop: drawCrop(p, size); break;
    case Kind::Undo: drawUndo(p, size); break;
    case Kind::Save: drawSave(p, size); break;
    case Kind::Close: drawClose(p, size); break;
    }

    p.end();
    return QIcon(pm);
}

QIcon colorSwatch(const QColor &color, int size)
{
    QPixmap pm = blankPixmap(size);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal m = size * 0.2;
    p.setPen(QPen(kStroke, size * 0.06));
    p.setBrush(color);
    p.drawEllipse(QRectF(m, m, size - 2 * m, size - 2 * m));
    p.end();
    return QIcon(pm);
}

} // namespace IconFactory
