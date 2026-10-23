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
#include "TextAnnotation.h"

#include <QColor>
#include <QFont>
#include <QImage>
#include <QRect>
#include <QVector>
#include <QWidget>
#include <functional>

class QTextCharFormat;
class QTextEdit;

// Draws a captured image plus vector annotations on top, and handles the
// mouse interactions for drawing shapes and selecting a crop rect. Crop is
// a two-step action: drag to propose a rect, then applyCrop() commits it
// (so it can be previewed/cancelled by switching tools first).
//
// The Text tool works on rich-text blocks: click empty space (or drag out a
// width) to start typing, click a block to select it, drag to move it, drag its
// right-edge handle to change its wrapping width, and click a selected block or
// double-click to edit it again. Text is edited in place in a QTextEdit laid
// over the canvas and is committed (and undoable) when editing ends.
class EditorCanvas : public QWidget {
    Q_OBJECT
public:
    enum class Tool { Arrow, Rectangle, Ellipse, Crop, Blur, Pixelate, Text };

    explicit EditorCanvas(const QImage &image, QWidget *parent = nullptr);

    void setTool(Tool tool);
    void setColor(const QColor &color);
    void setStrokeWidth(int width);

    // Defaults for newly created text blocks.
    void setTextDefaults(const QFont &font);
    // Apply one property to the text being edited (the selected characters, or
    // what's typed next), else to the whole selected block.
    void setTextFamily(const QString &family);
    void setTextPixelSize(int pixels);
    void setTextBold(bool on);
    void setTextItalic(bool on);
    void setTextUnderline(bool on);
    void recolorText(const QColor &color);
    // Finishes any in-progress text edit; call before reading the image.
    void commitTextEdit();

    bool hasPendingCrop() const { return m_hasPendingCrop; }
    void applyCrop();

    bool canUndo() const { return !m_undoStack.isEmpty(); }
    void undo();

    QImage flattenedImage() const;

signals:
    void changed();
    // The font/color of the text under the cursor or selection changed.
    void textFormatChanged(const QFont &font, const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class CropHandle { None, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left, Move };

    struct UndoEntry {
        QImage image;
        QVector<Annotation> annotations;
        QVector<TextAnnotation> texts;
    };
    void pushUndoSnapshot();
    UndoEntry currentState() const;
    void pushUndoEntry(const UndoEntry &entry);

    enum class TextDrag { None, Move, Resize, Create };
    int textAt(const QPoint &pos) const;
    bool onResizeHandle(const QPoint &pos) const;
    QRectF resizeHandleRect(const TextAnnotation &t) const;
    void textPress(QMouseEvent *event);
    void textMove(QMouseEvent *event);
    void textRelease(QMouseEvent *event);
    void selectText(int index);
    void startEditing(int index);
    void startNewText(const QPointF &pos, qreal width);
    void openEditor(const QString &html, const QPointF &pos, qreal width);
    void fitEditor();
    void ensureEditor();
    void deleteSelectedText();
    void applyTextModifier(const std::function<void(QTextCharFormat &)> &modify);
    void emitFormat(const QTextCharFormat &format);
    void emitSelectedTextFormat();
    QTextCharFormat defaultTextFormat() const;

    CropHandle cropHandleAt(const QPoint &pos) const;
    static QRect resizedCropRect(const QRect &orig, CropHandle handle, const QPoint &delta);
    void updateCropCursor(const QPoint &pos);

    QImage m_baseImage;
    QVector<Annotation> m_annotations;
    QVector<UndoEntry> m_undoStack;

    QVector<TextAnnotation> m_texts;
    int m_selectedText = -1;
    QTextEdit *m_editor = nullptr;
    bool m_editing = false;
    int m_editingIndex = -1; // -1 while typing a block that doesn't exist yet
    QPointF m_editPos;
    qreal m_editWidth = 0;
    UndoEntry m_editSnapshot;
    QFont m_textFont;

    TextDrag m_textDrag = TextDrag::None;
    QPoint m_textPress;
    QPointF m_textPosAtPress;
    qreal m_textWidthAtPress = 0;
    bool m_textMoved = false;
    bool m_pressedOnSelected = false;

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
