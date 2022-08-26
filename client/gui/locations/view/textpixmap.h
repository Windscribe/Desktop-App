#pragma once

#include "textpixmap.h"
#include "graphicresources/independentpixmap.h"

namespace gui_locations {

// Util class: convert the text to QPixmap to speed up subsequent drawings
// The reason for this is that drawing text is quite slow and it speeds up significantly
// todo: move to common utils?
class TextPixmap
{
public:
    explicit TextPixmap(const QString &text, const QFont &font, qreal devicePixelRatio);
    const IndependentPixmap *getPixmap() const { return pixmap_.get(); }
private:
    QScopedPointer<IndependentPixmap> pixmap_;
};

} // namespace gui_locations

