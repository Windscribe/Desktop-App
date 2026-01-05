#include "linkitem.h"

#include <QDesktopServices>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QUrl>

#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "dpiscalemanager.h"
#include "preferencesconst.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

LinkItem::LinkItem(ScalableGraphicsObject *parent, LinkType type, const QString &title, const QString &url)
  : BaseItem(parent, type == LinkType::TEXT_ONLY ? 40*G_SCALE : PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE),
    title_(title), url_(url), linkText_(""), type_(type), icon_(nullptr), linkIcon_(nullptr), inProgress_(false), spinnerRotation_(0), isTitleElided_(false), yOffset_(0)
{
    curArrowOpacity_= OPACITY_SIXTY;
    if (type == LinkType::TEXT_ONLY) {
        curTextOpacity_ = OPACITY_FULL;
    } else {
        setClickable(true);
        curTextOpacity_ = OPACITY_SIXTY;
        connect(this, &BaseItem::clicked, this, &LinkItem::onOpenUrl);
        connect(this, &BaseItem::hoverEnter, this, &LinkItem::onHoverEnter);
        connect(this, &BaseItem::hoverLeave, this, &LinkItem::onHoverLeave);
        connect(&textOpacityAnimation_, &QVariantAnimation::valueChanged, this, &LinkItem::onTextOpacityChanged);
        connect(&arrowOpacityAnimation_, &QVariantAnimation::valueChanged, this, &LinkItem::onArrowOpacityChanged);
        connect(&spinnerAnimation_, &QVariantAnimation::valueChanged, this, &LinkItem::onSpinnerRotationChanged);
        connect(&spinnerAnimation_, &QVariantAnimation::finished, this, &LinkItem::onSpinnerRotationFinished);
    }

    infoIcon_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/INFO_ICON", "", this, OPACITY_SIXTY);
    connect(infoIcon_, &IconButton::clicked, this, &LinkItem::onInfoIconClicked);

    setAcceptHoverEvents(true);
}

void LinkItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // link text
    QFont font = FontManager::instance().getFont(14, QFont::DemiBold);
    QFontMetrics fm(font);
    painter->setFont(font);
    painter->setPen(Qt::white);

    // Reset elided flags
    isTitleElided_ = false;

    // link text
    painter->setOpacity(curTextOpacity_);

    int linkTextWidth = CommonGraphics::textWidth(linkText_, font);
    int linkTextPosX = boundingRect().width() - linkTextWidth - PREFERENCES_MARGIN_X*G_SCALE;
    if (type_ == LinkType::EXTERNAL_LINK || type_ == LinkType::SUBPAGE_LINK) {
        linkTextPosX -= 19*G_SCALE;
    }
    painter->drawText(boundingRect().adjusted(linkTextPosX, yOffset_*G_SCALE, -PREFERENCES_MARGIN_X*G_SCALE, yOffset_*G_SCALE - (boundingRect().height() - PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)), Qt::AlignLeft | Qt::AlignVCenter, linkText_);

    // arrow or external link
    QSharedPointer<IndependentPixmap> p;
    if (inProgress_) {
        painter->setOpacity(1.0);
        p = ImageResourcesSvg::instance().getIndependentPixmap("SPINNER");
        painter->save();
        painter->translate(static_cast<int>(boundingRect().width() - p->width()/2 - PREFERENCES_MARGIN_X*G_SCALE), PREFERENCES_ITEM_Y*G_SCALE + p->height()/2);
        painter->rotate(spinnerRotation_);
        p->draw(-p->width()/2, -p->height()/2, painter);
        painter->restore();
    } else {
        painter->setOpacity(curArrowOpacity_);
        if (type_ == LinkType::EXTERNAL_LINK) {
            p = linkIcon_ ? linkIcon_ : ImageResourcesSvg::instance().getIndependentPixmap("preferences/EXTERNAL_LINK_ICON");
            p->draw(static_cast<int>(boundingRect().width() - p->width() - PREFERENCES_MARGIN_X*G_SCALE), PREFERENCES_ITEM_Y*G_SCALE, painter);
        } else if (type_ == LinkType::SUBPAGE_LINK) {
            p = linkIcon_ ? linkIcon_ : ImageResourcesSvg::instance().getIndependentPixmap("preferences/FRWRD_ARROW_WHITE_ICON");
            p->draw(static_cast<int>(boundingRect().width() - p->width() - PREFERENCES_MARGIN_X*G_SCALE), PREFERENCES_ITEM_Y*G_SCALE, painter);
        }
    }

    // text
    int xOffset = PREFERENCES_MARGIN_X*G_SCALE;
    if (icon_) {
        painter->setOpacity(OPACITY_FULL);
        icon_->draw(PREFERENCES_MARGIN_X*G_SCALE, ((PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT) / 2 + yOffset_)*G_SCALE, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
        xOffset = (PREFERENCES_MARGIN_X + 8 + ICON_WIDTH)*G_SCALE;
    } else {
        painter->setOpacity(curTextOpacity_);
    }

    QString title = title_;
    int availableTitleWidth = linkTextPosX - xOffset;

    if (availableTitleWidth < fm.horizontalAdvance(title_)) {
        title = fm.elidedText(title_, Qt::ElideRight, availableTitleWidth);
        isTitleElided_ = true;
    }

    // Store the title rectangle for mouse hover detection
    titleRect_ = QRectF(
        xOffset,
        (PREFERENCES_ITEM_Y + yOffset_)*G_SCALE,
        fm.horizontalAdvance(title),
        fm.height()
    );

    painter->drawText(boundingRect().adjusted(xOffset,
                                              yOffset_*G_SCALE,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                              yOffset_*G_SCALE - (boundingRect().height() - PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      title);

    // If there's a description draw it
    if (!desc_.isEmpty()) {
        QFont font = FontManager::instance().getFont(12, QFont::Normal);
        painter->setFont(font);
        painter->setPen(Qt::white);
        painter->setOpacity(OPACITY_SIXTY);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                                  boundingRect().height() - PREFERENCES_MARGIN_Y*G_SCALE - descHeight_,
                                                  -descRightMargin_,
                                                  0), Qt::AlignLeft | Qt::TextWordWrap, desc_);
    }
}

void LinkItem::hideOpenPopups()
{
    // overwrite
}

QString LinkItem::title()
{
    return title_;
}

void LinkItem::setTitle(const QString &title)
{
    title_ = title;
    updatePositions();
}

QString LinkItem::linkText()
{
    return linkText_;
}

void LinkItem::setLinkText(const QString &text)
{
    linkText_ = text;
    updatePositions();
}

void LinkItem::updateScaling()
{
    BaseItem::updateScaling();
    updatePositions();
}

void LinkItem::onHoverEnter()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(arrowOpacityAnimation_, curArrowOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void LinkItem::onHoverLeave()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(arrowOpacityAnimation_, curArrowOpacity_, OPACITY_SIXTY, ANIMATION_SPEED_FAST);
}

void LinkItem::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void LinkItem::onArrowOpacityChanged(const QVariant &value)
{
    curArrowOpacity_ = value.toDouble();
    update();
}

void LinkItem::setUrl(const QString &url)
{
    url_ = url;
}

void LinkItem::onOpenUrl()
{
    // this call does nothing when url is empty string, as is the case for edit account details
    QDesktopServices::openUrl(QUrl(url_));
}

void LinkItem::setIcon(QSharedPointer<IndependentPixmap> icon)
{
    icon_ = icon;
    updatePositions();
}

void LinkItem::setLinkIcon(QSharedPointer<IndependentPixmap> icon)
{
    linkIcon_ = icon;
    updatePositions();
}

void LinkItem::setInProgress(bool inProgress)
{
    inProgress_ = inProgress;
    if (inProgress_) {
        spinnerRotation_ = 0;
        startAnAnimation<int>(spinnerAnimation_, spinnerRotation_, 360, ANIMATION_SPEED_VERY_SLOW);
    } else {
        spinnerAnimation_.stop();
        update();
    }
}

void LinkItem::onSpinnerRotationChanged(const QVariant &value)
{
    spinnerRotation_ = value.toInt();
    update();
}

void LinkItem::onSpinnerRotationFinished()
{
    startAnAnimation<int>(spinnerAnimation_, 0, 360, ANIMATION_SPEED_VERY_SLOW);
}

void LinkItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isTitleElided_ && titleRect_.contains(event->pos())) {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(view->mapFromScene(scenePos()));

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        ti.x = globalPt.x() + titleRect_.x() + titleRect_.width()/2;
        ti.y = globalPt.y();
        ti.title = title_;
        if (scene() && !scene()->views().isEmpty()) {
            ti.parent = scene()->views().first()->viewport();
        }

        TooltipController::instance().showTooltipBasic(ti);
    } else {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    }

    BaseItem::hoverMoveEvent(event);
}

void LinkItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    BaseItem::hoverLeaveEvent(event);
}

void LinkItem::updatePositions()
{
    if (desc_.isEmpty()) {
        if (type_ == LinkType::TEXT_ONLY) {
            setHeight(40*G_SCALE);
            yOffset_ = -(PREFERENCE_GROUP_ITEM_HEIGHT - 40)/2;
        } else {
            setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
            yOffset_ = 0;
        }

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

    if (type_ == LinkType::TEXT_ONLY) {
        setHeight(40*G_SCALE + descHeight_ + DESCRIPTION_MARGIN*G_SCALE);
        yOffset_ = -(PREFERENCE_GROUP_ITEM_HEIGHT - 40)/2;
    } else {
        setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE + descHeight_ + DESCRIPTION_MARGIN*G_SCALE);
        yOffset_ = 0;
    }
    update();
}

void LinkItem::setDescription(const QString &description, const QString &url)
{
    desc_ = description;
    descUrl_ = url;
    updatePositions();
}

void LinkItem::onInfoIconClicked()
{
    QDesktopServices::openUrl(QUrl(descUrl_));
}

} // namespace PreferencesWindow
