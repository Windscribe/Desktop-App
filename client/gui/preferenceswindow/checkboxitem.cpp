#include "checkboxitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

CheckBoxItem::CheckBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip) : BaseItem(parent, 50*G_SCALE),
    strCaption_(caption), strTooltip_(tooltip)
{
    checkBoxButton_ = new CheckBoxButton(this);
    connect(checkBoxButton_, SIGNAL(stateChanged(bool)), SIGNAL(stateChanged(bool)));
    connect(checkBoxButton_, SIGNAL(hoverEnter()), SIGNAL(buttonHoverEnter()));
    connect(checkBoxButton_, SIGNAL(hoverLeave()), SIGNAL(buttonHoverLeave()));

    line_ = new DividerLine(this, 276);
    line_->setPos(24*G_SCALE, (50 - 3)*G_SCALE);

    updateScaling();
}

void CheckBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 0, 255)));
    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, -3*G_SCALE), Qt::AlignVCenter, tr(strCaption_.toStdString().c_str()));
}

void CheckBoxItem::setState(bool isChecked)
{
    checkBoxButton_->setState(isChecked);
}

bool CheckBoxItem::isChecked() const
{
    return checkBoxButton_->isChecked();
}

void CheckBoxItem::setLineVisible(bool visible)
{
    if (visible)
    {
        line_->show();
    }
    else
    {
        line_->hide();
    }
}

QPointF CheckBoxItem::getButtonScenePos() const
{
    return checkBoxButton_->scenePos();
}

void CheckBoxItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    checkBoxButton_->setPos(boundingRect().width() - checkBoxButton_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - checkBoxButton_->boundingRect().height()) / 2);
    line_->setPos(24*G_SCALE, (50 - 3)*G_SCALE);
}


} // namespace PreferencesWindow
