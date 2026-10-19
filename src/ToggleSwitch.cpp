/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        ToggleSwitch.cpp
 *  Description: Painting for the on/off slider switch
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

#include "ToggleSwitch.h"

#include <QPainter>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(40, 22);
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF track = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QColor trackColor = isChecked() ? QColor(61, 133, 198) : QColor(130, 130, 130);
    if (!isEnabled()) {
        trackColor.setAlpha(110);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(track, track.height() / 2, track.height() / 2);

    const qreal d = track.height() - 6;
    const qreal x = isChecked() ? track.right() - 3 - d : track.left() + 3;
    p.setBrush(Qt::white);
    p.drawEllipse(QRectF(x, track.top() + 3, d, d));
}
