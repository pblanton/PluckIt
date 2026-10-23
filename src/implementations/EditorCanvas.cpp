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

#include <QAbstractTextDocumentLayout>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <cmath>

namespace {
constexpr qreal kMinTextWidth = 24;
constexpr int kTextHandleSize = 8;
constexpr int kTextDragThreshold = 3;
}

EditorCanvas::EditorCanvas(const QImage &image, QWidget *parent)
    : QWidget(parent)
    , m_baseImage(image)
{
    setFixedSize(m_baseImage.size());
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    m_textFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    m_textFont.setPixelSize(24);
}

void EditorCanvas::setTool(Tool tool)
{
    commitTextEdit();
    m_tool = tool;
    m_hasPendingCrop = false; // switching tools discards an unconfirmed crop selection
    if (tool != Tool::Text) {
        m_selectedText = -1;
        unsetCursor();
    } else {
        setCursor(Qt::IBeamCursor);
        emitFormat(defaultTextFormat());
    }
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

EditorCanvas::UndoEntry EditorCanvas::currentState() const
{
    return {m_baseImage, m_annotations, m_texts};
}

void EditorCanvas::pushUndoEntry(const UndoEntry &entry)
{
    m_undoStack.push_back(entry);
    if (m_undoStack.size() > 50) {
        m_undoStack.pop_front();
    }
}

void EditorCanvas::pushUndoSnapshot()
{
    pushUndoEntry(currentState());
}

void EditorCanvas::applyCrop()
{
    if (!m_hasPendingCrop || m_pendingCropRect.isEmpty()) {
        return;
    }
    commitTextEdit();
    pushUndoSnapshot();

    const QRect rect = m_pendingCropRect.normalized().intersected(m_baseImage.rect());
    m_baseImage = m_baseImage.copy(rect);
    for (Annotation &a : m_annotations) {
        a.start -= rect.topLeft();
        a.end -= rect.topLeft();
    }
    for (TextAnnotation &t : m_texts) {
        t.pos -= rect.topLeft();
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
    commitTextEdit();
    if (m_undoStack.isEmpty()) {
        return;
    }
    const UndoEntry entry = m_undoStack.takeLast();
    m_baseImage = entry.image;
    m_annotations = entry.annotations;
    m_texts = entry.texts;
    m_selectedText = -1;
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
    for (const TextAnnotation &t : m_texts) {
        t.draw(painter);
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
    for (int i = 0; i < m_texts.size(); ++i) {
        if (!(m_editing && i == m_editingIndex)) {
            m_texts[i].draw(painter);
        }
    }

    if (m_tool == Tool::Text) {
        painter.setBrush(Qt::NoBrush);
        if (m_editing) {
            const QRectF r = m_editor->geometry();
            painter.setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
            painter.drawRect(r.adjusted(-1, -1, 1, 1));
        } else if (m_selectedText >= 0 && m_selectedText < m_texts.size()) {
            const TextAnnotation &t = m_texts[m_selectedText];
            painter.setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
            painter.drawRect(t.boundingRect());
            painter.setPen(QPen(Qt::black, 1));
            painter.setBrush(Qt::white);
            painter.drawRect(resizeHandleRect(t));
        }
        if (m_textDrag == TextDrag::Create) {
            painter.setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
            painter.setBrush(QColor(0, 120, 215, 30));
            painter.drawRect(QRect(m_textPress, m_dragCurrent).normalized());
        }
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
    if (m_tool == Tool::Text) {
        textPress(event);
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
    if (m_tool == Tool::Text) {
        textMove(event);
        return;
    }
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
    if (m_tool == Tool::Text) {
        textRelease(event);
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

QTextCharFormat EditorCanvas::defaultTextFormat() const
{
    QTextCharFormat format;
    format.setFont(m_textFont);
    format.setForeground(m_color);
    return format;
}

void EditorCanvas::setTextDefaults(const QFont &font)
{
    m_textFont = font;
}

void EditorCanvas::emitFormat(const QTextCharFormat &format)
{
    QFont font = format.font();
    if (format.hasProperty(QTextFormat::FontPixelSize)) {
        font.setPixelSize(format.intProperty(QTextFormat::FontPixelSize));
    } else if (format.fontPointSize() > 0) {
        font.setPixelSize(qRound(format.fontPointSize() * 96.0 / 72.0));
    } else {
        font.setPixelSize(m_textFont.pixelSize());
    }
    const QBrush brush = format.foreground();
    emit textFormatChanged(font, brush.style() == Qt::NoBrush ? m_color : brush.color());
}

void EditorCanvas::emitSelectedTextFormat()
{
    if (m_selectedText < 0 || m_selectedText >= m_texts.size()) {
        return;
    }
    QTextDocument doc;
    TextAnnotation::configure(doc, m_texts[m_selectedText].html, m_texts[m_selectedText].width);
    QTextCursor cursor(&doc);
    emitFormat(cursor.charFormat());
}

void EditorCanvas::applyTextModifier(const std::function<void(QTextCharFormat &)> &modify)
{
    QTextCharFormat modifier;
    modify(modifier);

    if (m_editing) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.mergeCharFormat(modifier);
        m_editor->mergeCurrentCharFormat(modifier);
        m_editor->setFocus();
    } else if (m_selectedText >= 0 && m_selectedText < m_texts.size()) {
        pushUndoSnapshot();
        QTextDocument doc;
        TextAnnotation::configure(doc, m_texts[m_selectedText].html, m_texts[m_selectedText].width);
        QTextCursor cursor(&doc);
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(modifier);
        m_texts[m_selectedText].html = doc.toHtml();
        emit changed();
        update();
    }
}

void EditorCanvas::setTextFamily(const QString &family)
{
    m_textFont.setFamily(family);
    applyTextModifier([&](QTextCharFormat &f) { f.setFontFamilies({family}); });
}

void EditorCanvas::setTextPixelSize(int pixels)
{
    m_textFont.setPixelSize(pixels);
    applyTextModifier([&](QTextCharFormat &f) { f.setProperty(QTextFormat::FontPixelSize, pixels); });
}

void EditorCanvas::setTextBold(bool on)
{
    m_textFont.setBold(on);
    applyTextModifier([&](QTextCharFormat &f) { f.setFontWeight(on ? QFont::Bold : QFont::Normal); });
}

void EditorCanvas::setTextItalic(bool on)
{
    m_textFont.setItalic(on);
    applyTextModifier([&](QTextCharFormat &f) { f.setFontItalic(on); });
}

void EditorCanvas::setTextUnderline(bool on)
{
    m_textFont.setUnderline(on);
    applyTextModifier([&](QTextCharFormat &f) { f.setFontUnderline(on); });
}

void EditorCanvas::recolorText(const QColor &color)
{
    applyTextModifier([&](QTextCharFormat &f) { f.setForeground(color); });
}

int EditorCanvas::textAt(const QPoint &pos) const
{
    for (int i = m_texts.size() - 1; i >= 0; --i) {
        if (m_texts[i].boundingRect().adjusted(-3, -3, 3, 3).contains(pos)) {
            return i;
        }
    }
    return -1;
}

QRectF EditorCanvas::resizeHandleRect(const TextAnnotation &t) const
{
    const QRectF r = t.boundingRect();
    return QRectF(r.right() - kTextHandleSize / 2.0, r.center().y() - kTextHandleSize / 2.0, kTextHandleSize,
                  kTextHandleSize);
}

bool EditorCanvas::onResizeHandle(const QPoint &pos) const
{
    if (m_selectedText < 0 || m_selectedText >= m_texts.size()) {
        return false;
    }
    return resizeHandleRect(m_texts[m_selectedText]).adjusted(-3, -3, 3, 3).contains(pos);
}

void EditorCanvas::selectText(int index)
{
    m_selectedText = index;
    if (index >= 0) {
        emitSelectedTextFormat();
    }
    update();
}

void EditorCanvas::textPress(QMouseEvent *event)
{
    if (m_editing) {
        commitTextEdit();
    }
    setFocus();

    const QPoint pos = event->pos();
    m_textPress = pos;
    m_dragCurrent = pos;
    m_textMoved = false;
    m_pressedOnSelected = false;

    if (onResizeHandle(pos)) {
        m_textDrag = TextDrag::Resize;
        m_textWidthAtPress = m_texts[m_selectedText].width > 0 ? m_texts[m_selectedText].width
                                                                : m_texts[m_selectedText].boundingRect().width();
        return;
    }

    const int hit = textAt(pos);
    if (hit >= 0) {
        m_pressedOnSelected = hit == m_selectedText;
        if (!m_pressedOnSelected) {
            selectText(hit);
        }
        m_textDrag = TextDrag::Move;
        m_textPosAtPress = m_texts[hit].pos;
        return;
    }

    m_selectedText = -1;
    m_textDrag = TextDrag::Create;
    update();
}

void EditorCanvas::textMove(QMouseEvent *event)
{
    const QPoint pos = event->pos();

    if (m_textDrag == TextDrag::None) {
        if (onResizeHandle(pos)) {
            setCursor(Qt::SizeHorCursor);
        } else {
            setCursor(textAt(pos) >= 0 ? Qt::SizeAllCursor : Qt::IBeamCursor);
        }
        return;
    }

    m_dragCurrent = pos;
    const QPoint delta = pos - m_textPress;
    if (!m_textMoved && delta.manhattanLength() > kTextDragThreshold) {
        m_textMoved = true;
        if (m_textDrag != TextDrag::Create) {
            pushUndoSnapshot();
        }
    }

    if (m_textMoved && m_selectedText >= 0 && m_selectedText < m_texts.size()) {
        if (m_textDrag == TextDrag::Move) {
            m_texts[m_selectedText].pos = m_textPosAtPress + delta;
        } else if (m_textDrag == TextDrag::Resize) {
            m_texts[m_selectedText].width = qMax(kMinTextWidth, m_textWidthAtPress + delta.x());
        }
    }
    update();
}

void EditorCanvas::textRelease(QMouseEvent *event)
{
    const TextDrag drag = m_textDrag;
    m_textDrag = TextDrag::None;

    if (drag == TextDrag::Create) {
        const QRect r = QRect(m_textPress, event->pos()).normalized();
        const bool dragged = r.width() >= 8;
        startNewText(dragged ? QPointF(r.topLeft()) : QPointF(m_textPress.x(), m_textPress.y()),
                     dragged ? r.width() : 0);
        return;
    }

    if (m_textMoved) {
        emit changed();
        update();
        return;
    }

    if (drag == TextDrag::Move && m_pressedOnSelected && m_selectedText >= 0) {
        startEditing(m_selectedText);
    }
    update();
}

void EditorCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_tool == Tool::Text && event->button() == Qt::LeftButton) {
        const int hit = textAt(event->pos());
        if (hit >= 0) {
            startEditing(hit);
        }
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void EditorCanvas::keyPressEvent(QKeyEvent *event)
{
    if (m_tool == Tool::Text && !m_editing && m_selectedText >= 0) {
        switch (event->key()) {
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            deleteSelectedText();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_F2:
            startEditing(m_selectedText);
            return;
        case Qt::Key_Escape:
            m_selectedText = -1;
            update();
            return;
        default:
            break;
        }
    }
    QWidget::keyPressEvent(event);
}

void EditorCanvas::hideEvent(QHideEvent *event)
{
    commitTextEdit();
    QWidget::hideEvent(event);
}

void EditorCanvas::deleteSelectedText()
{
    if (m_selectedText < 0 || m_selectedText >= m_texts.size()) {
        return;
    }
    pushUndoSnapshot();
    m_texts.removeAt(m_selectedText);
    m_selectedText = -1;
    emit changed();
    update();
}

void EditorCanvas::ensureEditor()
{
    if (m_editor) {
        return;
    }
    m_editor = new QTextEdit(this);
    m_editor->setFrameShape(QFrame::NoFrame);
    m_editor->setStyleSheet(QStringLiteral("QTextEdit { background: transparent; }"));
    m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_editor->setAcceptRichText(true);
    m_editor->document()->setDocumentMargin(TextAnnotation::kMargin);
    m_editor->installEventFilter(this);
    m_editor->hide();

    connect(m_editor->document(), &QTextDocument::contentsChanged, this, [this]() {
        if (m_editing) {
            fitEditor();
        }
    });
    connect(m_editor, &QTextEdit::currentCharFormatChanged, this, [this](const QTextCharFormat &format) {
        if (m_editing) {
            emitFormat(format);
        }
    });
}

void EditorCanvas::fitEditor()
{
    QTextDocument *doc = m_editor->document();
    int w;
    if (m_editWidth > 0) {
        w = qRound(m_editWidth);
        doc->setTextWidth(m_editWidth);
    } else {
        doc->setTextWidth(-1);
        // A little slack so the caret at the end of a line isn't clipped.
        w = qMax(30, qCeil(doc->idealWidth()) + 6);
    }
    const int h = qMax(qCeil(doc->size().height()), m_textFont.pixelSize() + 6);
    m_editor->setGeometry(QRect(m_editPos.toPoint(), QSize(w, h)));
}

void EditorCanvas::openEditor(const QString &html, const QPointF &pos, qreal width)
{
    ensureEditor();
    m_editPos = pos;
    m_editWidth = width;
    m_editor->setLineWrapMode(width > 0 ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);

    m_editing = true;
    if (html.isEmpty()) {
        m_editor->clear();
        m_editor->document()->setDefaultFont(m_textFont);
        m_editor->setCurrentCharFormat(defaultTextFormat());
    } else {
        m_editor->setHtml(html);
        m_editor->moveCursor(QTextCursor::End);
    }
    fitEditor();
    m_editor->show();
    m_editor->setFocus();
    emitFormat(m_editor->currentCharFormat());
    update();
}

void EditorCanvas::startEditing(int index)
{
    if (index < 0 || index >= m_texts.size()) {
        return;
    }
    commitTextEdit();
    m_selectedText = index;
    m_editingIndex = index;
    m_editSnapshot = currentState();
    openEditor(m_texts[index].html, m_texts[index].pos, m_texts[index].width);
}

void EditorCanvas::startNewText(const QPointF &pos, qreal width)
{
    commitTextEdit();
    m_selectedText = -1;
    m_editingIndex = -1;
    m_editSnapshot = currentState();
    openEditor(QString(), pos, width);
}

void EditorCanvas::commitTextEdit()
{
    if (!m_editing) {
        return;
    }
    m_editing = false;
    m_editor->hide();

    const bool empty = m_editor->document()->toPlainText().trimmed().isEmpty();
    const QString html = m_editor->document()->toHtml();
    const int index = m_editingIndex;
    m_editingIndex = -1;

    bool modified = false;
    if (index < 0) {
        if (!empty) {
            TextAnnotation t;
            t.html = html;
            t.pos = m_editPos;
            t.width = m_editWidth;
            pushUndoEntry(m_editSnapshot);
            m_texts.push_back(t);
            m_selectedText = m_texts.size() - 1;
            modified = true;
        }
    } else if (index < m_texts.size()) {
        if (empty) {
            pushUndoEntry(m_editSnapshot);
            m_texts.removeAt(index);
            m_selectedText = -1;
            modified = true;
        } else if (html != m_texts[index].html) {
            pushUndoEntry(m_editSnapshot);
            m_texts[index].html = html;
            modified = true;
        }
    }

    setFocus();
    if (modified) {
        emit changed();
    }
    update();
}

bool EditorCanvas::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_editor && event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            commitTextEdit();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
