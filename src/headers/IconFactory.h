/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        IconFactory.h
 *  Description: Procedurally drawn toolbar icons and widget glyphs
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
#include <QIcon>
#include <QRectF>

class QPainter;

// Small procedurally-drawn toolbar icons. Deliberately not relying on the
// desktop's icon theme (QIcon::fromTheme) since icon availability/naming
// varies a lot across GNOME/KDE/XFCE -- drawing our own guarantees a
// consistent look everywhere.
namespace IconFactory {

enum class Kind { Arrow, Rectangle, Ellipse, Pixelate, Blur, Crop, Undo, Save, Close };

// Outline glyphs for the floating capture widget. Unlike the toolbar icons
// these take a color (the widget is dark, the editor toolbar is light) and are
// painted as vectors so they stay crisp on HiDPI.
enum class Glyph { Camera, Gear, Pencil, Menu };

QIcon icon(Kind kind, int size = 28);
void paintGlyph(QPainter &painter, Glyph glyph, const QRectF &rect, const QColor &color);
QIcon glyphIcon(Glyph glyph, int size, const QColor &color);
QIcon colorSwatch(const QColor &color, int size = 28);

} // namespace IconFactory
