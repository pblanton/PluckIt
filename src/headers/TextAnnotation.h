//HDR: Rich-text annotation block drawn onto a capture
#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>

class QPainter;
class QTextDocument;

// A block of rich text placed on the image. The content is stored as the HTML
// that QTextDocument produces, which keeps per-run font, size, style and color,
// so a single block can mix them. Sizes inside it are pixels, not points, so
// the block renders the same on the widget and in the flattened image
// regardless of the screen's DPI.
struct TextAnnotation {
    // Padding between the block's edge and its text; the in-place editor uses
    // the same value so text doesn't shift when editing starts or ends.
    static constexpr qreal kMargin = 3;

    QString html;
    QPointF pos;
    qreal width = 0; // 0 means no wrapping: the block is as wide as its text

    static void configure(QTextDocument &doc, const QString &html, qreal width);

    QSizeF size() const;
    QRectF boundingRect() const { return QRectF(pos, size()); }
    void draw(QPainter &painter) const;
};
