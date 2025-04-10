#include "toggleitem.h"

#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>

#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "preferencesconst.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

ToggleItem::ToggleItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), strCaption_(caption), strTooltip_(tooltip),
    captionFont_(12, true), icon_(nullptr), isCaptionElided_(false)
{
    checkBoxButton_ = new ToggleButton(this);
    connect(checkBoxButton_, &ToggleButton::stateChanged, this, &ToggleItem::stateChanged);
    connect(checkBoxButton_, &ToggleButton::hoverEnter, this, &ToggleItem::buttonHoverEnter);
    connect(checkBoxButton_, &ToggleButton::hoverLeave, this, &ToggleItem::buttonHoverLeave);

    setAcceptHoverEvents(true);
    updateScaling();
}

void ToggleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(captionFont_);
    painter->setFont(font);
    painter->setPen(Qt::white);

    qreal xOffset = PREFERENCES_MARGIN*G_SCALE;
    if (icon_) {
        xOffset = (1.5*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE;
        icon_->draw(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
    }

    QFontMetrics fm(font);
    int availableWidth = checkBoxButton_->pos().x() - xOffset - DESCRIPTION_MARGIN*G_SCALE;
    QString elidedText = strCaption_;

    // Reset elided flag
    isCaptionElided_ = false;

    if (availableWidth < fm.horizontalAdvance(strCaption_)) {
        elidedText = fm.elidedText(strCaption_, Qt::ElideRight, availableWidth, 0);
        isCaptionElided_ = true;
    }

    // Store the caption rectangle for mouse hover detection
    captionRect_ = QRectF(
        xOffset,
        PREFERENCES_MARGIN*G_SCALE,
        fm.horizontalAdvance(elidedText),
        fm.height()
    );

    painter->drawText(boundingRect().adjusted(xOffset,
                                              PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE),
                      Qt::AlignLeft,
                      elidedText);
}

void ToggleItem::setEnabled(bool enabled)
{
    BaseItem::setEnabled(enabled);
    checkBoxButton_->setEnabled(enabled);
}

void ToggleItem::setState(bool isChecked)
{
    checkBoxButton_->setState(isChecked);
}

bool ToggleItem::isChecked() const
{
    return checkBoxButton_->isChecked();
}

QPointF ToggleItem::getButtonScenePos() const
{
    return checkBoxButton_->scenePos();
}

void ToggleItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    checkBoxButton_->setPos(boundingRect().width() - checkBoxButton_->boundingRect().width() - PREFERENCES_MARGIN*G_SCALE,
                            CHECKBOX_MARGIN_Y*G_SCALE);
}

void ToggleItem::setIcon(QSharedPointer<IndependentPixmap> icon)
{
    icon_ = icon;
}

void ToggleItem::setCaptionFont(const FontDescr &fontDescr)
{
    captionFont_ = fontDescr;
}

void ToggleItem::setCaption(const QString &caption)
{
    strCaption_ = caption;
    update();
}

void ToggleItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isCaptionElided_ && captionRect_.contains(event->pos())) {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(view->mapFromScene(scenePos()));

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.x = globalPt.x() + captionRect_.x() + captionRect_.width()/2;
        ti.y = globalPt.y();
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        ti.title = strCaption_;

        TooltipController::instance().showTooltipBasic(ti);
    } else {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    }

    BaseItem::hoverMoveEvent(event);
}

void ToggleItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    BaseItem::hoverLeaveEvent(event);
}

} // namespace PreferencesWindow
