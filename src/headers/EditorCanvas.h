/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        EditorCanvas.h
 *  Description: Canvas widget for drawing annotations and cropping a captured image
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

#include "Annotation.h"

#include <QColor>
#include <QImage>
#include <QRect>
#include <QVector>
#include <QWidget>

// Draws a captured image plus vector annotations on top, and handles the
// mouse interactions for drawing shapes and selecting a crop rect. Crop is
// a two-step action: drag to propose a rect, then applyCrop() commits it
// (so it can be previewed/cancelled by switching tools first).
class EditorCanvas : public QWidget {
    Q_OBJECT
public:
    enum class Tool { Arrow, Rectangle, Ellipse, Crop, Blur, Pixelate };

    explicit EditorCanvas(const QImage &image, QWidget *parent = nullptr);

    void setTool(Tool tool);
    void setColor(const QColor &color);
    void setStrokeWidth(int width);

    bool hasPendingCrop() const { return m_hasPendingCrop; }
    void applyCrop();

    bool canUndo() const { return !m_undoStack.isEmpty(); }
    void undo();

    QImage flattenedImage() const;

signals:
    void changed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    enum class CropHandle { None, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left, Move };

    struct UndoEntry {
        QImage image;
        QVector<Annotation> annotations;
    };
    void pushUndoSnapshot();

    CropHandle cropHandleAt(const QPoint &pos) const;
    static QRect resizedCropRect(const QRect &orig, CropHandle handle, const QPoint &delta);
    void updateCropCursor(const QPoint &pos);

    QImage m_baseImage;
    QVector<Annotation> m_annotations;
    QVector<UndoEntry> m_undoStack;

    Tool m_tool = Tool::Arrow;
    QColor m_color = QColor(220, 40, 40);
    int m_strokeWidth = 3;

    bool m_dragging = false;
    QPoint m_dragStart;
    QPoint m_dragCurrent;

    bool m_hasPendingCrop = false;
    QRect m_pendingCropRect;

    bool m_resizingCrop = false;
    CropHandle m_activeCropHandle = CropHandle::None;
    QRect m_cropRectAtDragStart;
};
