#include "checkboxitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "preferencesconst.h"

namespace PreferencesWindow {

CheckBoxItem::CheckBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), strCaption_(caption), strTooltip_(tooltip),
    captionFont_(12, true), icon_(nullptr)
{
    checkBoxButton_ = new CheckBoxButton(this);
    connect(checkBoxButton_, &CheckBoxButton::stateChanged, this, &CheckBoxItem::stateChanged);
    connect(checkBoxButton_, &CheckBoxButton::hoverEnter, this, &CheckBoxItem::buttonHoverEnter);
    connect(checkBoxButton_, &CheckBoxButton::hoverLeave, this, &CheckBoxItem::buttonHoverLeave);

    updateScaling();
}

void CheckBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont *font = FontManager::instance().getFont(captionFont_);
    painter->setFont(*font);
    painter->setPen(Qt::white);

    int xOffset = PREFERENCES_MARGIN*G_SCALE;
    if (icon_)
    {
        xOffset = (2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE;
        icon_->draw(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
    }
    painter->drawText(boundingRect().adjusted(xOffset,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      Qt::AlignLeft,
                      strCaption_);
}

void CheckBoxItem::setEnabled(bool enabled)
{
    BaseItem::setEnabled(enabled);
    checkBoxButton_->setEnabled(enabled);
}

void CheckBoxItem::setState(bool isChecked)
{
    checkBoxButton_->setState(isChecked);
}

bool CheckBoxItem::isChecked() const
{
    return checkBoxButton_->isChecked();
}

QPointF CheckBoxItem::getButtonScenePos() const
{
    return checkBoxButton_->scenePos();
}

void CheckBoxItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    checkBoxButton_->setPos(boundingRect().width() - checkBoxButton_->boundingRect().width() - PREFERENCES_MARGIN*G_SCALE,
                            CHECKBOX_MARGIN_Y*G_SCALE);
}

void CheckBoxItem::setIcon(QSharedPointer<IndependentPixmap> icon)
{
    icon_ = icon;
}

void CheckBoxItem::setCaptionFont(const FontDescr &fontDescr)
{
    captionFont_ = fontDescr;
}

} // namespace PreferencesWindow
