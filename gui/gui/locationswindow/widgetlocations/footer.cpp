#include "footer.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"


namespace GuiLocations {

Footer::Footer(QWidget *parent) : QWidget(parent)
{

}

void Footer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QColor footerColor_ = FontManager::instance().getLocationsFooterColor();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // currently just 2px slice to cover the last viewport item line in the locations list
#ifdef Q_OS_MAC
        p.setPen(footerColor_);
        p.setBrush(footerColor_);
        p.drawRoundedRect(QRect(0, 0, WINDOW_WIDTH *G_SCALE, FOOTER_HEIGHT * G_SCALE), 8, 8);
        p.fillRect(QRect(0, 0, WINDOW_WIDTH *G_SCALE, (FOOTER_HEIGHT - 2) * G_SCALE / 2), QBrush(footerColor_));
#else
        p.fillRect(QRect(0, 0, WINDOW_WIDTH *G_SCALE, (FOOTER_HEIGHT - 2) * G_SCALE), QBrush(footerColor_));
#endif

        // let owner handle for now
    // draw footer handle
//    QRect middle(width() / 2 - 12* G_SCALE,
//                 height() - FOOTER_HEIGHT * G_SCALE / 2 - COVER_LAST_ITEM_LINE * G_SCALE,
//                 24 * G_SCALE, 3 * G_SCALE);
//    p.setOpacity(0.5);
//    p.fillRect(QRect(middle.left(), middle.top(), middle.width(), 2 * G_SCALE), QBrush(Qt::white));
}

}
