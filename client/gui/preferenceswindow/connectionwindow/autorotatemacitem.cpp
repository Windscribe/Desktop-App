#include "autorotatemacitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AutoRotateMacItem::AutoRotateMacItem(ScalableGraphicsObject *parent) : BaseItem(parent, 43)
{
    checkBoxButton_ = new CheckBoxButton(this);
    connect(checkBoxButton_, SIGNAL(stateChanged(bool)), SIGNAL(stateChanged(bool)));

    dividerLine = new DividerLine(this, 276);

    strCaption_ = QT_TR_NOOP("Auto-rotate MAC on every new network connection");
    updatePositions();
}

void AutoRotateMacItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->fillRect(boundingRect().adjusted(24*G_SCALE, 0, 0, 0), QBrush(QColor(16, 22, 40)));

    QFont *font = FontManager::instance().getFont(12, true);

    QFontMetrics fm(*font);
    QRect captionRect = fm.boundingRect(boundingRect().adjusted((16 + 24)*G_SCALE, 0, -32*G_SCALE - checkBoxButton_->boundingRect().width() , -3*G_SCALE).toRect(), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, strCaption_);

    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(captionRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, tr(strCaption_.toStdString().c_str()));
}

void AutoRotateMacItem::setState(bool isChecked)
{
    checkBoxButton_->setState(isChecked);
}

void AutoRotateMacItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(43*G_SCALE);
    updatePositions();
}

void AutoRotateMacItem::updatePositions()
{
    checkBoxButton_->setPos(boundingRect().width() - checkBoxButton_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - checkBoxButton_->boundingRect().height()) / 2);
    dividerLine->setPos(24*G_SCALE, boundingRect().height() - 3*G_SCALE);
}


} // namespace PreferencesWindow
