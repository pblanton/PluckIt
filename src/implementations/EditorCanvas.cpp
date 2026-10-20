/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        EditorCanvas.cpp
 *  Description: Mouse handling, painting, and redaction tools for the editor canvas
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

#include "../headers/EditorCanvas.h"

#include "../headers/ImageEffects.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

EditorCanvas::EditorCanvas(const QImage &image, QWidget *parent)
    : QWidget(parent)
    , m_baseImage(image)
{
    setFixedSize(m_baseImage.size());
    setMouseTracking(true);
}

void EditorCanvas::setTool(Tool tool)
{
    m_tool = tool;
    m_hasPendingCrop = false; // switching tools discards an unconfirmed crop selection
    update();
}

void EditorCanvas::setColor(const QColor &color)
{
    m_color = color;
}

void EditorCanvas::setStrokeWidth(int width)
{
    m_strokeWidth = width;
}

EditorCanvas::CropHandle EditorCanvas::cropHandleAt(const QPoint &pos) const
{
    if (!m_hasPendingCrop) {
        return CropHandle::None;
    }
    const QRect r = m_pendingCropRect.normalized();
    const QPoint tl = r.topLeft();
    const QPoint tr = r.topRight();
    const QPoint bl = r.bottomLeft();
    const QPoint br = r.bottomRight();
    const QPoint topMid((tl.x() + tr.x()) / 2, tl.y());
    const QPoint bottomMid((bl.x() + br.x()) / 2, bl.y());
    const QPoint leftMid(tl.x(), (tl.y() + bl.y()) / 2);
    const QPoint rightMid(tr.x(), (tr.y() + br.y()) / 2);

    struct HandlePoint {
        CropHandle handle;
        QPoint point;
    };
    const HandlePoint handles[] = {
        {CropHandle::TopLeft, tl},     {CropHandle::Top, topMid},        {CropHandle::TopRight, tr},
        {CropHandle::Right, rightMid}, {CropHandle::BottomRight, br},    {CropHandle::Bottom, bottomMid},
        {CropHandle::BottomLeft, bl},  {CropHandle::Left, leftMid},
    };

    constexpr int kHandleRadius = 8;
    for (const HandlePoint &h : handles) {
        const QPoint d = h.point - pos;
        if (d.x() * d.x() + d.y() * d.y() <= kHandleRadius * kHandleRadius) {
            return h.handle;
        }
    }

    if (r.adjusted(kHandleRadius, kHandleRadius, -kHandleRadius, -kHandleRadius).contains(pos)) {
        return CropHandle::Move;
    }
    return CropHandle::None;
}

QRect EditorCanvas::resizedCropRect(const QRect &orig, CropHandle handle, const QPoint &delta)
{
    QRect r = orig;
    switch (handle) {
    case CropHandle::TopLeft: r.setTopLeft(orig.topLeft() + delta); break;
    case CropHandle::Top: r.setTop(orig.top() + delta.y()); break;
    case CropHandle::TopRight: r.setTopRight(orig.topRight() + delta); break;
    case CropHandle::Right: r.setRight(orig.right() + delta.x()); break;
    case CropHandle::BottomRight: r.setBottomRight(orig.bottomRight() + delta); break;
    case CropHandle::Bottom: r.setBottom(orig.bottom() + delta.y()); break;
    case CropHandle::BottomLeft: r.setBottomLeft(orig.bottomLeft() + delta); break;
    case CropHandle::Left: r.setLeft(orig.left() + delta.x()); break;
    case CropHandle::Move: r.translate(delta); break;
    case CropHandle::None: break;
    }
    return r.normalized();
}

void EditorCanvas::updateCropCursor(const QPoint &pos)
{
    switch (cropHandleAt(pos)) {
    case CropHandle::TopLeft:
    case CropHandle::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case CropHandle::TopRight:
    case CropHandle::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case CropHandle::Top:
    case CropHandle::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case CropHandle::Left:
    case CropHandle::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case CropHandle::Move:
        setCursor(Qt::SizeAllCursor);
        break;
    case CropHandle::None:
    default:
        setCursor(Qt::CrossCursor);
        break;
    }
}

void EditorCanvas::pushUndoSnapshot()
{
    m_undoStack.push_back({m_baseImage, m_annotations});
    if (m_undoStack.size() > 50) {
        m_undoStack.pop_front();
    }
}

void EditorCanvas::applyCrop()
{
    if (!m_hasPendingCrop || m_pendingCropRect.isEmpty()) {
        return;
    }
    pushUndoSnapshot();

    const QRect rect = m_pendingCropRect.normalized().intersected(m_baseImage.rect());
    m_baseImage = m_baseImage.copy(rect);
    for (Annotation &a : m_annotations) {
        a.start -= rect.topLeft();
        a.end -= rect.topLeft();
    }
    m_hasPendingCrop = false;
    m_pendingCropRect = QRect();

    setFixedSize(m_baseImage.size());
    updateGeometry();
    emit changed();
    update();
}

void EditorCanvas::undo()
{
    if (m_undoStack.isEmpty()) {
        return;
    }
    const UndoEntry entry = m_undoStack.takeLast();
    m_baseImage = entry.image;
    m_annotations = entry.annotations;
    m_hasPendingCrop = false;

    setFixedSize(m_baseImage.size());
    updateGeometry();
    emit changed();
    update();
}

QImage EditorCanvas::flattenedImage() const
{
    QImage result = m_baseImage.copy();
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    for (const Annotation &a : m_annotations) {
        a.draw(painter);
    }
    return result;
}

void EditorCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawImage(0, 0, m_baseImage);
    painter.setRenderHint(QPainter::Antialiasing);

    for (const Annotation &a : m_annotations) {
        a.draw(painter);
    }

    const bool isShapeTool = m_tool == Tool::Arrow || m_tool == Tool::Rectangle || m_tool == Tool::Ellipse;
    if (m_dragging && isShapeTool) {
        Annotation preview;
        preview.type = m_tool == Tool::Rectangle ? AnnotationType::Rectangle
                      : m_tool == Tool::Ellipse  ? AnnotationType::Ellipse
                                                  : AnnotationType::Arrow;
        preview.start = m_dragStart;
        preview.end = m_dragCurrent;
        preview.color = m_color;
        preview.strokeWidth = m_strokeWidth;
        preview.draw(painter);
    }

    if (m_dragging && (m_tool == Tool::Blur || m_tool == Tool::Pixelate)) {
        const QRect previewRect = QRect(m_dragStart, m_dragCurrent).normalized();
        painter.setPen(QPen(Qt::yellow, 1, Qt::DashLine));
        painter.setBrush(QColor(255, 255, 0, 40));
        painter.drawRect(previewRect);
    }

    if (m_hasPendingCrop || (m_dragging && m_tool == Tool::Crop)) {
        const QRect cropRect = m_dragging ? QRect(m_dragStart, m_dragCurrent).normalized()
                                           : m_pendingCropRect;

        QPainterPath dim;
        dim.addRect(rect());
        dim.addRect(cropRect);
        dim.setFillRule(Qt::OddEvenFill);
        painter.fillPath(dim, QColor(0, 0, 0, 120));

        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(cropRect);
    }

    if (m_hasPendingCrop && !m_dragging) {
        const QRect r = m_pendingCropRect.normalized();
        const QPoint handlePositions[] = {
            r.topLeft(),
            QPoint((r.left() + r.right()) / 2, r.top()),
            r.topRight(),
            QPoint(r.right(), (r.top() + r.bottom()) / 2),
            r.bottomRight(),
            QPoint((r.left() + r.right()) / 2, r.bottom()),
            r.bottomLeft(),
            QPoint(r.left(), (r.top() + r.bottom()) / 2),
        };

        constexpr int kHandleSize = 7;
        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(Qt::white);
        for (const QPoint &p : handlePositions) {
            painter.drawRect(QRect(p.x() - kHandleSize / 2, p.y() - kHandleSize / 2, kHandleSize, kHandleSize));
        }
    }
}

void EditorCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (m_tool == Tool::Crop && m_hasPendingCrop) {
        const CropHandle handle = cropHandleAt(event->pos());
        if (handle != CropHandle::None) {
            m_resizingCrop = true;
            m_activeCropHandle = handle;
            m_cropRectAtDragStart = m_pendingCropRect.normalized();
            m_dragStart = event->pos();
            update();
            return;
        }
    }

    m_dragStart = event->pos();
    m_dragCurrent = event->pos();
    m_dragging = true;
    update();
}

void EditorCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_resizingCrop) {
        const QPoint delta = event->pos() - m_dragStart;
        m_pendingCropRect = resizedCropRect(m_cropRectAtDragStart, m_activeCropHandle, delta);
        update();
        return;
    }

    if (m_dragging) {
        m_dragCurrent = event->pos();
        update();
        return;
    }

    if (m_tool == Tool::Crop && m_hasPendingCrop) {
        updateCropCursor(event->pos());
    }
}

void EditorCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (m_resizingCrop) {
        m_resizingCrop = false;
        m_activeCropHandle = CropHandle::None;
        m_pendingCropRect = m_pendingCropRect.normalized();
        emit changed();
        update();
        return;
    }

    if (!m_dragging) {
        return;
    }
    m_dragging = false;

    const QRect dragRect = QRect(m_dragStart, m_dragCurrent).normalized();
    if (dragRect.width() < 3 || dragRect.height() < 3) {
        update();
        return;
    }

    if (m_tool == Tool::Crop) {
        m_pendingCropRect = dragRect;
        m_hasPendingCrop = true;
        emit changed();
    } else if (m_tool == Tool::Blur || m_tool == Tool::Pixelate) {
        pushUndoSnapshot();
        if (m_tool == Tool::Blur) {
            ImageEffects::applyBoxBlur(m_baseImage, dragRect);
        } else {
            ImageEffects::applyPixelate(m_baseImage, dragRect);
        }
        emit changed();
    } else {
        pushUndoSnapshot();
        Annotation a;
        a.type = m_tool == Tool::Rectangle ? AnnotationType::Rectangle
                : m_tool == Tool::Ellipse  ? AnnotationType::Ellipse
                                            : AnnotationType::Arrow;
        a.start = m_dragStart;
        a.end = m_dragCurrent;
        a.color = m_color;
        a.strokeWidth = m_strokeWidth;
        m_annotations.push_back(a);
        emit changed();
    }
    update();
}
