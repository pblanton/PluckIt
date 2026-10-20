/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        Annotation.h
 *  Description: Vector markup element (arrow, rectangle, ellipse) declaration
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

#include <QColor>
#include <QPoint>

class QPainter;

enum class AnnotationType { Arrow, Rectangle, Ellipse };

// A single vector markup element, drawn in image-local coordinates.
struct Annotation {
    AnnotationType type = AnnotationType::Arrow;
    QPoint start;
    QPoint end;
    QColor color = QColor(220, 40, 40);
    int strokeWidth = 3;

    void draw(QPainter &painter) const;
};
