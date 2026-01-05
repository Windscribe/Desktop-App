#include "statictext.h"

#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>

#include "dpiscalemanager.h"
#include "commongraphics/basepage.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/log/categories.h"


namespace PreferencesWindow {

StaticText::StaticText(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)
{
    setAcceptHoverEvents(true);
}

void StaticText::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(14, QFont::Normal);
    QFontMetrics fm(font);

    QString text = text_;
    QString caption = caption_;

    int textWidth = fm.horizontalAdvance(text);
    int captionWidth = fm.horizontalAdvance(caption);

    // Reset elided flags
    isCaptionElided_ = false;
    isTextElided_ = false;

    if (boundingRect().width() < textWidth + captionWidth + (2*PREFERENCES_MARGIN_X + 8)*G_SCALE) {
        if (textWidth < boundingRect().width()/2) {
            // text is less than half the width, so let's show all of it and elide the caption.
            caption = fm.elidedText(caption, Qt::ElideRight, boundingRect().width() - textWidth - (2*PREFERENCES_MARGIN_X + 8)*G_SCALE);
            isCaptionElided_ = true;
            captionWidth = fm.horizontalAdvance(caption);
        } else if (captionWidth < boundingRect().width()/2) {
            // caption is less than half the width, so let's show all of it and elide the text.
            text = fm.elidedText(text, Qt::ElideRight, boundingRect().width() - captionWidth - (2*PREFERENCES_MARGIN_X + 8)*G_SCALE);
            isTextElided_ = true;
            textWidth = fm.horizontalAdvance(text);
        } else {
            // text and caption are both too long, so let's elide both.
            text = fm.elidedText(text, Qt::ElideRight, boundingRect().width()/2 - (PREFERENCES_MARGIN_X + 4)*G_SCALE);
            caption = fm.elidedText(caption, Qt::ElideRight, boundingRect().width()/2 - (PREFERENCES_MARGIN_X + 4)*G_SCALE);
            isCaptionElided_ = true;
            isTextElided_ = true;
            textWidth = fm.horizontalAdvance(text);
        }
    }

    // Store the caption and text rectangles for mouse hover detection
    captionRect_ = QRectF(
        PREFERENCES_MARGIN_X*G_SCALE,
        PREFERENCES_MARGIN_Y*G_SCALE,
        captionWidth,
        fm.height()
    );

    textRect_ = QRectF(
        boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE - textWidth,
        PREFERENCES_MARGIN_Y*G_SCALE,
        textWidth,
        fm.height()
    );

    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              0,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                              0),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      caption);

    painter->setOpacity(OPACITY_SIXTY);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              0,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                              0),
                      Qt::AlignRight | Qt::AlignVCenter,
                      text);
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

void StaticText::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(scenePos()));

    if (isCaptionElided_ && captionRect_.contains(event->pos())) {
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        ti.title = caption_;
        ti.x = globalPt.x() + captionRect_.x() + captionRect_.width()/2;
        ti.y = globalPt.y();
        if (scene() && !scene()->views().isEmpty()) {
            ti.parent = scene()->views().first()->viewport();
        }
        TooltipController::instance().showTooltipBasic(ti);
    } else if (isTextElided_ && textRect_.contains(event->pos())) {
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        ti.title = text_;
        ti.x = globalPt.x() + textRect_.x() + textRect_.width()/2;
        ti.y = globalPt.y();
        if (scene() && !scene()->views().isEmpty()) {
            ti.parent = scene()->views().first()->viewport();
        }
        TooltipController::instance().showTooltipBasic(ti);
    } else {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    }

    BaseItem::hoverMoveEvent(event);
}

void StaticText::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    // Hide tooltip when mouse leaves the item
    TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    BaseItem::hoverLeaveEvent(event);
}

} // namespace PreferencesWindow
