//HDR: Layout and painting for rich-text annotation blocks
#include "../headers/TextAnnotation.h"

#include <QPainter>
#include <QTextDocument>

void TextAnnotation::configure(QTextDocument &doc, const QString &html, qreal width)
{
    doc.setDocumentMargin(kMargin);
    doc.setHtml(html);
    doc.setTextWidth(width > 0 ? width : -1);
}

QSizeF TextAnnotation::size() const
{
    QTextDocument doc;
    configure(doc, html, width);
    return QSizeF(width > 0 ? width : doc.idealWidth(), doc.size().height());
}

void TextAnnotation::draw(QPainter &painter) const
{
    QTextDocument doc;
    configure(doc, html, width);
    painter.save();
    painter.translate(pos);
    doc.drawContents(&painter);
    painter.restore();
}
