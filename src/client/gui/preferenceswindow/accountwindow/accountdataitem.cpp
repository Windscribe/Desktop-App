#include "accountdataitem.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

AccountDataItem::AccountDataItem(ScalableGraphicsObject *parent, const QString &value1, const QString &value2)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), value1_(value1), value2_(value2)
{
}

void AccountDataItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font1 = FontManager::instance().getFont(14, QFont::DemiBold);
    QFontMetrics fm1(font1);
    painter->setFont(font1);
    painter->setOpacity(OPACITY_FULL);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, 0), Qt::AlignLeft | Qt::AlignVCenter, value1_);

    int remainingSpace = boundingRect().width() - fm1.horizontalAdvance(value1_) - (2*PREFERENCES_MARGIN_X + APP_ICON_MARGIN_X)*G_SCALE;

    QFont font2 = FontManager::instance().getFont(14, QFont::DemiBold);
    QFontMetrics fm2(font2);
    painter->setFont(font2);
    painter->setOpacity(OPACITY_SIXTY);
    QString elidedText = value2_;
    if (fm2.horizontalAdvance(value2_) > remainingSpace)
    {
        elidedText = fm2.elidedText(value2_, Qt::TextElideMode::ElideRight, remainingSpace);
    }
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, 0), Qt::AlignRight | Qt::AlignVCenter, elidedText);
}

void AccountDataItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
}

void AccountDataItem::setValue1(const QString &value)
{
    value1_ = value;
    update();
}

void AccountDataItem::setValue2(const QString &value)
{
    value2_ = value;
    update();
}

} // namespace PreferencesWindow
