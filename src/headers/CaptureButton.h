/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureButton.h
 *  Description: Round red capture button with a mode-menu chevron and countdown display
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

#include <QAbstractButton>

// The main button of the capture widget. A plain click captures with the
// current mode; the small chevron in its lower-right corner (or a right-click)
// asks for the mode menu instead. While a capture delay is counting down it
// shows the remaining seconds in place of the camera glyph.
class CaptureButton : public QAbstractButton {
    Q_OBJECT
public:
    explicit CaptureButton(QWidget *parent = nullptr);

    void setCountdown(int seconds);
    QSize sizeHint() const override;

signals:
    void modeMenuRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool hitButton(const QPoint &pos) const override;

private:
    bool inChevronZone(const QPoint &pos) const;

    int m_countdown = 0;
    bool m_chevronPressed = false;
};
