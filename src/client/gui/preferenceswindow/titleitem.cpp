#include "titleitem.h"

#include <QCursor>
#include <QFont>
#include <QPainter>

#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "preferencesconst.h"

namespace PreferencesWindow {

TitleItem::TitleItem(ScalableGraphicsObject *parent, const QString &title)
  : BaseItem(parent, ACCOUNT_TITLE_HEIGHT*G_SCALE), title_(title), linkText_(""), linkColor_(FontManager::instance().getSeaGreenColor()), currentLinkColor_(FontManager::instance().getSeaGreenColor())
{
    setAcceptHoverEvents(true);
    connect(&colorAnimation_, &QVariantAnimation::valueChanged, this, &TitleItem::onColorAnimationValueChanged);
}

void TitleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(12, QFont::DemiBold);
    qreal oldLetterSpacing = font.letterSpacing();
    font.setLetterSpacing(QFont::AbsoluteSpacing, 1.44);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_SEVENTY);
    painter->drawText(boundingRect().adjusted(0, 0, -PREFERENCES_MARGIN_X*G_SCALE, 0), Qt::AlignLeft, title_);

    if (!linkText_.isEmpty()) {
        painter->setOpacity(OPACITY_FULL);
        painter->setPen(currentLinkColor_);
        int width = QFontMetrics(font).horizontalAdvance(linkText_);
        linkRect_ = QRectF(boundingRect().width() - width, 0, width, ACCOUNT_TITLE_HEIGHT*G_SCALE);
        painter->drawText(linkRect_, Qt::AlignRight, linkText_);
    }

    font.setLetterSpacing(QFont::AbsoluteSpacing, oldLetterSpacing);
}

void TitleItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(ACCOUNT_TITLE_HEIGHT*G_SCALE);
}

void TitleItem::setTitle(const QString &title)
{
    title_ = title;
}

void TitleItem::setLink(const QString &linkText)
{
    linkText_ = linkText;
    update();
}

void TitleItem::hover()
{
    setCursor(Qt::PointingHandCursor);
    startAnAnimation<double>(colorAnimation_, animProgress_, 1, ANIMATION_SPEED_FAST);
}

void TitleItem::unhover()
{
    setCursor(Qt::ArrowCursor);
    startAnAnimation<double>(colorAnimation_, animProgress_, 0, ANIMATION_SPEED_FAST);
}

void TitleItem::onColorAnimationValueChanged(const QVariant &value)
{
    animProgress_ = value.toDouble();
    currentLinkColor_ = QColor(85 + 170*animProgress_, 255, 138 + 117*animProgress_);
    update();
}

void TitleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (linkRect_.contains(event->pos())) {
        emit linkClicked();
    }
}

void TitleItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (linkRect_.contains(event->pos())) {
        hover();
    } else {
        unhover();
    }
}

void TitleItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    unhover();
}

} // namespace PreferencesWindow
