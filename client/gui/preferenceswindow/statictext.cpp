#include "statictext.h"

#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "dpiscalemanager.h"
#include "commongraphics/basepage.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "tooltips/tooltipcontroller.h"


namespace PreferencesWindow {

StaticText::StaticText(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)
{
}

void StaticText::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(12, false);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -(2*PREFERENCES_MARGIN)*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      Qt::AlignLeft,
                      caption_);

    painter->setOpacity(OPACITY_HALF);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -(2*PREFERENCES_MARGIN)*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      Qt::AlignRight,
                      text_);
}

void StaticText::setCaption(const QString &caption)
{
    caption_ = caption;
    update();
}

void StaticText::setText(const QString &text)
{
    text_ = text;
    update();
}

void StaticText::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
}

} // namespace PreferencesWindow

