/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        IconFactory.cpp
 *  Description: Painter-based implementations of the icons and glyphs
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

#include "../headers/IconFactory.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <cmath>

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

void drawCopy(QPainter &p, qreal size)
{
    const qreal w = size * 0.07;
    p.setPen(QPen(kStroke, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    // back sheet: only its top and left edges peek out from behind the front one
    const QRectF back(size * 0.2, size * 0.14, size * 0.46, size * 0.52);
    QPainterPath backPath;
    backPath.moveTo(back.left() + size * 0.12, back.top());
    backPath.lineTo(back.left() + size * 0.04, back.top());
    backPath.quadTo(back.topLeft(), QPointF(back.left(), back.top() + size * 0.04));
    backPath.lineTo(back.left(), back.bottom() - size * 0.02);
    p.drawPath(backPath);
    p.drawLine(QPointF(back.left() + size * 0.12, back.top()), QPointF(back.right(), back.top()));
    p.drawLine(QPointF(back.right(), back.top()), QPointF(back.right(), back.top() + size * 0.08));

    const QRectF front(size * 0.34, size * 0.3, size * 0.46, size * 0.54);
    p.setBrush(QColor(255, 255, 255, 0));
    p.drawRoundedRect(front, size * 0.06, size * 0.06);
}

void drawLetter(QPainter &p, qreal size, const QString &letter, bool bold, bool italic, bool underline)
{
    QFont font(QStringLiteral("Sans Serif"));
    font.setPixelSize(qRound(size * 0.72));
    font.setBold(bold);
    font.setItalic(italic);
    p.setFont(font);
    p.setPen(kStroke);
    const QRectF box(0, size * (underline ? -0.04 : 0.0), size, size);
    p.drawText(box, Qt::AlignCenter, letter);
    if (underline) {
        p.setPen(QPen(kStroke, size * 0.07, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(size * 0.24, size * 0.86), QPointF(size * 0.76, size * 0.86));
    }
}

void drawTextTool(QPainter &p, qreal size)
{
    // capital "T" with serifs, drawn from lines so it stays crisp at any size
    p.setPen(QPen(kStroke, size * 0.1, Qt::SolidLine, Qt::SquareCap));
    p.drawLine(QPointF(size * 0.2, size * 0.22), QPointF(size * 0.8, size * 0.22));
    p.drawLine(QPointF(size * 0.5, size * 0.22), QPointF(size * 0.5, size * 0.8));
    p.setPen(QPen(kStroke, size * 0.07, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(size * 0.2, size * 0.22), QPointF(size * 0.2, size * 0.34));
    p.drawLine(QPointF(size * 0.8, size * 0.22), QPointF(size * 0.8, size * 0.34));
    p.drawLine(QPointF(size * 0.38, size * 0.8), QPointF(size * 0.62, size * 0.8));
}

void drawClose(QPainter &p, qreal size)
{
    const qreal m = size * 0.26;
    p.setPen(QPen(kStroke, size * 0.1, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(m, m), QPointF(size - m, size - m));
    p.drawLine(QPointF(size - m, m), QPointF(m, size - m));
}

void drawCamera(QPainter &p, const QRectF &r, const QColor &color)
{
    const qreal s = r.width();
    p.setPen(QPen(color, s * 0.07, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    QPainterPath body;
    body.addRoundedRect(QRectF(r.left() + s * 0.1, r.top() + s * 0.28, s * 0.8, s * 0.56), s * 0.1, s * 0.1);
    p.drawPath(body);

    QPainterPath bump;
    bump.moveTo(r.left() + s * 0.32, r.top() + s * 0.28);
    bump.lineTo(r.left() + s * 0.38, r.top() + s * 0.16);
    bump.lineTo(r.left() + s * 0.62, r.top() + s * 0.16);
    bump.lineTo(r.left() + s * 0.68, r.top() + s * 0.28);
    p.drawPath(bump);

    p.drawEllipse(QPointF(r.left() + s * 0.5, r.top() + s * 0.56), s * 0.17, s * 0.17);
    p.setPen(QPen(color, s * 0.08, Qt::SolidLine, Qt::RoundCap));
    p.drawPoint(QPointF(r.left() + s * 0.78, r.top() + s * 0.39));
}

void drawGear(QPainter &p, const QRectF &r, const QColor &color)
{
    const qreal s = r.width();
    const QPointF c = r.center();
    const int teeth = 8;
    const qreal outer = s * 0.44;
    const qreal inner = s * 0.33;
    const qreal step = 2 * M_PI / teeth;
    const qreal half = step / 2;

    QPainterPath path;
    for (int i = 0; i < teeth; ++i) {
        const qreal base = i * step;
        const qreal pts[4][2] = {
            {base - half * 0.75, inner}, {base - half * 0.4, outer},
            {base + half * 0.4, outer}, {base + half * 0.75, inner},
        };
        for (int k = 0; k < 4; ++k) {
            const QPointF pt(c.x() + pts[k][1] * std::sin(pts[k][0]), c.y() - pts[k][1] * std::cos(pts[k][0]));
            if (i == 0 && k == 0) {
                path.moveTo(pt);
            } else {
                path.lineTo(pt);
            }
        }
    }
    path.closeSubpath();

    p.setPen(QPen(color, s * 0.07, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    p.drawEllipse(c, s * 0.14, s * 0.14);
}

void drawPencil(QPainter &p, const QRectF &r, const QColor &color)
{
    const qreal s = r.width();
    p.save();
    p.translate(r.center());
    p.rotate(45);
    p.translate(-s / 2, -s / 2);

    p.setPen(QPen(color, s * 0.07, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPolygonF outline;
    outline << QPointF(s * 0.38, s * 0.14) << QPointF(s * 0.62, s * 0.14) << QPointF(s * 0.62, s * 0.68)
            << QPointF(s * 0.5, s * 0.88) << QPointF(s * 0.38, s * 0.68);
    p.drawPolygon(outline);
    p.drawLine(QPointF(s * 0.38, s * 0.28), QPointF(s * 0.62, s * 0.28));
    p.drawLine(QPointF(s * 0.38, s * 0.68), QPointF(s * 0.62, s * 0.68));
    p.restore();
}

void drawMenu(QPainter &p, const QRectF &r, const QColor &color)
{
    const qreal s = r.width();
    p.setPen(QPen(color, s * 0.1, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 3; ++i) {
        const qreal y = r.top() + s * (0.28 + 0.22 * i);
        p.drawLine(QPointF(r.left() + s * 0.18, y), QPointF(r.right() - s * 0.18, y));
    }
}

} // namespace

void paintGlyph(QPainter &painter, Glyph glyph, const QRectF &rect, const QColor &color)
{
    const qreal side = qMin(rect.width(), rect.height());
    const QRectF square(rect.center().x() - side / 2, rect.center().y() - side / 2, side, side);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    switch (glyph) {
    case Glyph::Camera: drawCamera(painter, square, color); break;
    case Glyph::Gear: drawGear(painter, square, color); break;
    case Glyph::Pencil: drawPencil(painter, square, color); break;
    case Glyph::Menu: drawMenu(painter, square, color); break;
    }
    painter.restore();
}

QIcon glyphIcon(Glyph glyph, int size, const QColor &color)
{
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pm(qRound(size * dpr), qRound(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    paintGlyph(p, glyph, QRectF(0, 0, size, size), color);
    p.end();
    return QIcon(pm);
}

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
    case Kind::Copy: drawCopy(p, size); break;
    case Kind::Close: drawClose(p, size); break;
    case Kind::Text: drawTextTool(p, size); break;
    case Kind::Bold: drawLetter(p, size, QStringLiteral("B"), true, false, false); break;
    case Kind::Italic: drawLetter(p, size, QStringLiteral("I"), false, true, false); break;
    case Kind::Underline: drawLetter(p, size, QStringLiteral("U"), false, false, true); break;
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
