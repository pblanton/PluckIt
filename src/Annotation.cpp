/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        Annotation.cpp
 *  Description: Drawing implementation for vector markup elements
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

#include "Annotation.h"

#include <QPainter>
#include <QPolygonF>

#include <cmath>

void Annotation::draw(QPainter &painter) const
{
    QPen pen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    switch (type) {
    case AnnotationType::Rectangle:
        painter.drawRect(QRect(start, end).normalized());
        break;
    case AnnotationType::Ellipse:
        painter.drawEllipse(QRect(start, end).normalized());
        break;
    case AnnotationType::Arrow: {
        painter.drawLine(start, end);

        const double angle = std::atan2(double(end.y() - start.y()), double(end.x() - start.x()));
        const double arrowSize = 10 + strokeWidth * 2;
        const QPointF p1(end.x() - arrowSize * std::cos(angle - M_PI / 6),
                          end.y() - arrowSize * std::sin(angle - M_PI / 6));
        const QPointF p2(end.x() - arrowSize * std::cos(angle + M_PI / 6),
                          end.y() - arrowSize * std::sin(angle + M_PI / 6));

        QPolygonF head;
        head << QPointF(end) << p1 << p2;
        painter.setBrush(color);
        painter.drawPolygon(head);
        break;
    }
    }
}
