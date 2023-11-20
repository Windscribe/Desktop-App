#include "entryitem.h"

#include <QDateTime>
#include <QFontMetrics>
#include <QPainter>
#include <QTextDocument>

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"

namespace NewsFeedWindow {

EntryItem::EntryItem(ScalableGraphicsObject *parent, const types::Notification &item, int width)
    : BaseItem(parent, COLLAPSED_HEIGHT*G_SCALE, "", false, width), item_(item), expanded_(false), accented_(false), expandAnimationProgress_(0),
      opacityAnimationProgress_(0), textOpacity_(OPACITY_HALF), plusIconOpacity_(OPACITY_QUARTER), read_(false),
      icon_(ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON"))
{
    opacityAnimation_.setStartValue(0.0);
    opacityAnimation_.setEndValue(1.0);
    opacityAnimation_.setDuration(200);
    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &EntryItem::onOpacityAnimationValueChanged);

    expandAnimation_.setStartValue(0.0);
    expandAnimation_.setEndValue(1.0);
    expandAnimation_.setDuration(200);
    connect(&expandAnimation_, &QVariantAnimation::valueChanged, this, &EntryItem::onExpandRotationAnimationValueChanged);

    setClickable(true);
    setResetHoverOnClick(false);
    connect(this, &BaseItem::clicked, this, &EntryItem::onClicked);
    connect(this, &BaseItem::hoverEnter, this, &EntryItem::onHoverEnter);
    connect(this, &BaseItem::hoverLeave, this, &EntryItem::onHoverLeave);

    messageItem_ = new MessageItem(this, width, item_.message);

    updateScaling();
}

void EntryItem::setAccented(bool accented)
{
    if (accented != accented_)
    {
        if (accented)
        {
            opacityAnimation_.setDirection(QAbstractAnimation::Forward);
        }
        else
        {
            opacityAnimation_.setDirection(QAbstractAnimation::Backward);
        }
        accented_ = accented;
        opacityAnimation_.start();
    }
}

bool EntryItem::isExpanded() const
{
    return expanded_;
}

int EntryItem::collapsedHeight()
{
    return COLLAPSED_HEIGHT;
}

int EntryItem::expandedHeight()
{
    return expandedHeight_;
}

void EntryItem::setExpanded(bool expanded, bool read)
{
    if (expanded)
    {
        expandAnimation_.setDirection(QAbstractAnimation::Forward);
        plusIconOpacity_ = OPACITY_FULL;
        textOpacity_ = OPACITY_FULL;
        opacityAnimationProgress_ = OPACITY_FULL;
        accented_ = true;
        if (read)
        {
            setRead(true);
        }
    }
    else
    {
        expandAnimation_.setDirection(QAbstractAnimation::Backward);
    }
    expanded_ = expanded;
    expandAnimation_.start();
}

void EntryItem::setRead(bool read)
{
    if (read && !read_)
    {
        emit messageRead(item_.id);
    }
    read_ = read;
}

void EntryItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // rounded rectangle background
    painter->setOpacity(0.08);
    QPainterPath path;
    path.addRoundedRect(boundingRect(), ROUNDED_RECT_RADIUS*G_SCALE, ROUNDED_RECT_RADIUS*G_SCALE);
    painter->fillPath(path, QBrush(Qt::white));

    // unread marker
    if (!read_) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(85, 255, 138));
        painter->setOpacity(OPACITY_FULL);
        painter->drawEllipse(-(READ_MARKER_WIDTH/2)*G_SCALE,
                             (READ_MARKER_WIDTH/2 + TEXT_MARGIN)*G_SCALE,
                             READ_MARKER_WIDTH*G_SCALE,
                             READ_MARKER_WIDTH*G_SCALE);
    }

    // title
    painter->setPen(Qt::SolidLine);
    painter->setPen(Qt::white);
    painter->setOpacity(textOpacity_);
    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    if (expanded_) {
        painter->drawText(boundingRect().adjusted(TEXT_MARGIN*G_SCALE,
                                                  TEXT_MARGIN*G_SCALE,
                                                  -(2*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE),
                                                  0),
                          item_.title.toUpper());
    } else {
        QFontMetrics titleMetrics(*font);
        QString elided = titleMetrics.elidedText(item_.title.toUpper(),
                                                 Qt::ElideRight,
                                                 boundingRect().width() - (3*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE));
        painter->drawText(boundingRect().adjusted(TEXT_MARGIN*G_SCALE,
                                                  TEXT_MARGIN*G_SCALE,
                                                  -(2*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE),
                                                  0),
                          elided);
    }

    // +/X icon
    painter->save();
    painter->setOpacity(plusIconOpacity_);
    painter->translate(QPoint(boundingRect().width() - TEXT_MARGIN*G_SCALE - icon_->width()/2,
                       TEXT_MARGIN*G_SCALE + icon_->height()/2));
    painter->rotate(45 * expandAnimationProgress_);
    icon_->draw(-icon_->width()/2, -icon_->height()/2, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
    painter->restore();

    // body
    if (expanded_) {
        messageItem_->setVisible(true);
        messageItem_->setOpacity(expandAnimationProgress_);
    } else {
        messageItem_->setVisible(false);
    }
}

void EntryItem::updateScaling()
{
    BaseItem::updateScaling();
    QFontMetrics metrics(*FontManager::instance().getFont(12, true));
    titleHeight_ = metrics.boundingRect(boundingRect().adjusted(TEXT_MARGIN*G_SCALE,
                                                                TEXT_MARGIN*G_SCALE,
                                                                -(2*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE),
                                                                0).toRect(),
                                        Qt::AlignLeft | Qt::TextWordWrap,
                                        item_.title.toUpper()).height();
    expandedHeight_ = titleHeight_ + TEXT_MARGIN*G_SCALE + messageItem_->boundingRect().height();
    setHeight(COLLAPSED_HEIGHT*G_SCALE + expandAnimationProgress_ * (expandedHeight_ - COLLAPSED_HEIGHT*G_SCALE));

    updatePositions();
}

void EntryItem::updatePositions()
{
    messageItem_->setPos(0, titleHeight_ + TEXT_MARGIN*G_SCALE);
}

void EntryItem::onExpandRotationAnimationValueChanged(const QVariant &value)
{
    expandAnimationProgress_ = value.toDouble();
    setHeight(COLLAPSED_HEIGHT*G_SCALE + expandAnimationProgress_ * (expandedHeight_ - COLLAPSED_HEIGHT*G_SCALE));
    update();
}

void EntryItem::onOpacityAnimationValueChanged(const QVariant &value)
{
    opacityAnimationProgress_ = value.toDouble();
    plusIconOpacity_ = OPACITY_QUARTER + (opacityAnimationProgress_ * (1-OPACITY_QUARTER));
    textOpacity_ = OPACITY_HALF + (opacityAnimationProgress_ * (1-OPACITY_HALF));
    update();
}

void EntryItem::onClicked()
{
    setExpanded(!expanded_);

    if (expanded_)
    {
        emit scrollToItem(this);
    }
}

void EntryItem::onHoverEnter()
{
    setAccented(true);
}

void EntryItem::onHoverLeave()
{
    if (!expanded_)
    {
        setAccented(false);
    }
}

int EntryItem::id() const
{
    return item_.id;
}

void EntryItem::setItem(const types::Notification &item)
{
    item_ = item;
    delete messageItem_;
    messageItem_ = new MessageItem(this, boundingRect().width()/G_SCALE, item_.message);
    updatePositions();
    update();
}

bool EntryItem::isRead() const
{
    return read_;
}

} // namespace NewsFeedWindow
