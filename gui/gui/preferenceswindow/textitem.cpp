#include "textitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

TextItem::TextItem(ScalableGraphicsObject *parent, QString text, int height) : BaseItem(parent, height),
  text_(text),
  height_(height)
{

}

void TextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setPen(QColor(255, 255, 255));
    painter->setOpacity(0.5 * initOpacity);
    painter->setFont(*FontManager::instance().getFont(12, true));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 16*G_SCALE, -40*G_SCALE, 0), text_);
}

void TextItem::setText(QString text)
{
    text_ = text;
    update();
}

void TextItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(height_*G_SCALE);
}

}
