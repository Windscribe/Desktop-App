#pragma once

#include "textpixmap.h"
#include "graphicresources/independentpixmap.h"

#include <QHash>

namespace gui_locations {

// Util class: convert the text to QPixmap to speed up subsequent drawings
// The reason for this is that drawing text is quite slow and it speeds up significantly
// todo: move to common utils?
class TextPixmap
{
public:
    TextPixmap() {}
    explicit TextPixmap(const QString &text, const QFont &font, qreal devicePixelRatio);
    IndependentPixmap pixmap() const { return pixmap_; }
    QString text() const { return text_; }

private:
    IndependentPixmap pixmap_;
    QString text_;
};

// convenient for managing multiple text pixmaps
class TextPixmaps
{
public:
    TextPixmaps() {}
    void add(int id, const QString &text, const QFont &font, qreal devicePixelRatio);
    void updateIfTextChanged(int id, const QString &text, const QFont &font, qreal devicePixelRatio);
    IndependentPixmap pixmap(int id) const;

private:
    QHash<int, TextPixmap> pixmaps_;
};


} // namespace gui_locations

