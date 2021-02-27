#include "footertopstrip.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"


namespace GuiLocations {

FooterTopStrip::FooterTopStrip(QWidget *parent) : QWidget(parent)
{

}

void FooterTopStrip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QColor footerColor = FontManager::instance().getLocationsFooterColor();

    // just a 2px slice to cover the last viewport item line in the locations list
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRect geo(0, 0, WINDOW_WIDTH *G_SCALE, geometry().height());
    p.fillRect(geo, QBrush(footerColor));

}

}
