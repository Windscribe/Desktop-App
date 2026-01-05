#include "toggleitem.h"

#include <QCursor>
#include <QDesktopServices>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QUrl>

#include "utils/log/categories.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferencesconst.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

ToggleItem::ToggleItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), strCaption_(caption), strTooltip_(tooltip),
    captionFont_(14, QFont::Bold), icon_(nullptr), isCaptionElided_(false)
{
    checkBoxButton_ = new ToggleButton(this);
    connect(checkBoxButton_, &ToggleButton::stateChanged, this, &ToggleItem::stateChanged);
    connect(checkBoxButton_, &ToggleButton::hoverEnter, this, &ToggleItem::buttonHoverEnter);
    connect(checkBoxButton_, &ToggleButton::hoverLeave, this, &ToggleItem::buttonHoverLeave);

    infoIcon_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/INFO_ICON", "", this, OPACITY_SIXTY);
    connect(infoIcon_, &IconButton::clicked, this, &ToggleItem::onInfoIconClicked);

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

    qreal xOffset = PREFERENCES_MARGIN_X*G_SCALE;
    if (icon_) {
        xOffset = (PREFERENCES_MARGIN_X + 8 + ICON_WIDTH)*G_SCALE;
        icon_->draw(PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
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
        PREFERENCES_ITEM_Y*G_SCALE,
        fm.horizontalAdvance(elidedText),
        fm.height()
    );

    painter->drawText(boundingRect().adjusted(xOffset,
                                              0,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                              -(boundingRect().height() - PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      elidedText);

    // If there's a description draw it
    if (!desc_.isEmpty()) {
        QFont font = FontManager::instance().getFont(12, QFont::Normal);
        painter->setFont(font);
        painter->setPen(Qt::white);
        painter->setOpacity(OPACITY_SEVENTY);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                                  boundingRect().height() - PREFERENCES_MARGIN_Y*G_SCALE - descHeight_,
                                                  -descRightMargin_,
                                                  0), Qt::AlignLeft | Qt::TextWordWrap, desc_);
    }
}

void ToggleItem::setEnabled(bool enabled)
{
    BaseItem::setEnabled(enabled);
    checkBoxButton_->setEnabled(enabled);
    infoIcon_->setEnabled(true);
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
    checkBoxButton_->setPos(boundingRect().width() - checkBoxButton_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE,
                            CHECKBOX_MARGIN_Y*G_SCALE);
    updatePositions();
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
        if (scene() && !scene()->views().isEmpty()) {
            ti.parent = scene()->views().first()->viewport();
        }

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

void ToggleItem::setDescription(const QString &description, const QString &url)
{
    desc_ = description;
    descUrl_ = url;
    updatePositions();
}

void ToggleItem::updatePositions()
{
    if (desc_.isEmpty()) {
        setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
        infoIcon_->setVisible(false);
        return;
    }

    descRightMargin_ = PREFERENCES_MARGIN_X*G_SCALE;
    if (!descUrl_.isEmpty()) {
        descRightMargin_ += PREFERENCES_MARGIN_X*G_SCALE + ICON_WIDTH*G_SCALE;
    }

    QFontMetrics fm(FontManager::instance().getFont(12, QFont::Normal));
    descHeight_ = fm.boundingRect(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -descRightMargin_, 0).toRect(),
                                  Qt::AlignLeft | Qt::TextWordWrap,
                                  desc_).height();

    if (descUrl_.isEmpty()) {
        infoIcon_->setVisible(false);
    } else {
        infoIcon_->setVisible(true);
        infoIcon_->setPos(boundingRect().width() + PREFERENCES_MARGIN_X*G_SCALE - descRightMargin_,
                          boundingRect().height() - PREFERENCES_MARGIN_Y*G_SCALE - descHeight_/2 - ICON_HEIGHT*G_SCALE/2);
    }

    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE + descHeight_ + DESCRIPTION_MARGIN*G_SCALE);
    update();
}

void ToggleItem::onInfoIconClicked()
{
    QDesktopServices::openUrl(QUrl(descUrl_));
}

} // namespace PreferencesWindow
