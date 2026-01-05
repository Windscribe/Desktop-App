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

EntryItem::EntryItem(ScalableGraphicsObject *parent, const api_responses::Notification &item, int width)
    : BaseItem(parent, COLLAPSED_HEIGHT*G_SCALE, "", false, width), item_(item), expanded_(false), accented_(false), expandAnimationProgress_(0),
      opacityAnimationProgress_(0), textOpacity_(OPACITY_SIXTY), iconOpacity_(OPACITY_SIXTY), read_(false),
      icon_(new ImageItem(this, "ARROW_DOWN_WHITE"))
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

    icon_->setOpacity(iconOpacity_);

    updateScaling();
}

void EntryItem::setAccented(bool accented)
{
    if (accented != accented_) {
        if (accented) {
            opacityAnimation_.setDirection(QAbstractAnimation::Forward);
        } else {
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
    if (expanded) {
        expandAnimation_.setDirection(QAbstractAnimation::Forward);
        iconOpacity_ = OPACITY_FULL;
        textOpacity_ = OPACITY_FULL;
        opacityAnimationProgress_ = OPACITY_FULL;
        icon_->setOpacity(iconOpacity_);
        accented_ = true;
        if (read) {
            setRead(true);
        }
    } else {
        expandAnimation_.setDirection(QAbstractAnimation::Backward);
    }
    expanded_ = expanded;
    expandAnimation_.start();
}

void EntryItem::setRead(bool read)
{
    if (read && !read_) {
        emit messageRead(item_.id);
    }
    read_ = read;
}

void EntryItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // unread marker
    if (!read_) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(FontManager::instance().getSeaGreenColor());
        painter->setOpacity(OPACITY_FULL);
        painter->drawEllipse(-(READ_MARKER_WIDTH/2)*G_SCALE,
                             (COLLAPSED_HEIGHT/2 - READ_MARKER_WIDTH/2)*G_SCALE,
                             READ_MARKER_WIDTH*G_SCALE,
                             READ_MARKER_WIDTH*G_SCALE);
    }

    // title
    painter->setPen(Qt::SolidLine);
    painter->setPen(Qt::white);
    painter->setOpacity(textOpacity_);
    QFont font = FontManager::instance().getFont(14, QFont::Bold);
    painter->setFont(font);

    QFontMetrics titleMetrics(font);
    QString elided = titleMetrics.elidedText(item_.title.toUpper(), Qt::ElideRight, boundingRect().width() - (3*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE));
    painter->drawText(boundingRect().adjusted(TEXT_MARGIN*G_SCALE, TEXT_MARGIN*G_SCALE, -(2*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE), 0), elided);

    // date
    painter->setOpacity(OPACITY_FULL);
    font = FontManager::instance().getFont(14, QFont::Normal);
    painter->setPen(QColor(114, 121, 129));
    painter->setFont(font);
    QString date = QDateTime::fromSecsSinceEpoch(item_.date).toString(QLocale::system().dateFormat());
    painter->drawText(boundingRect().adjusted(TEXT_MARGIN*G_SCALE, TEXT_MARGIN_DATE*G_SCALE, -(2*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE), TEXT_MARGIN*G_SCALE), date);

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
    QFontMetrics metrics(FontManager::instance().getFont(14, QFont::Bold));
    titleHeight_ = metrics.boundingRect(boundingRect().adjusted(TEXT_MARGIN*G_SCALE,
                                                                TEXT_MARGIN*G_SCALE,
                                                                -(2*TEXT_MARGIN*G_SCALE + ICON_WIDTH*G_SCALE),
                                                                0).toRect(),
                                        Qt::AlignLeft | Qt::TextWordWrap,
                                        item_.title.toUpper()).height();
    expandedHeight_ = TEXT_MARGIN*G_SCALE + BODY_MARGIN*G_SCALE + messageItem_->boundingRect().height();
    icon_->setPos(boundingRect().width() - (TEXT_MARGIN + ICON_WIDTH)*G_SCALE, TEXT_MARGIN*G_SCALE);
    setHeight(COLLAPSED_HEIGHT*G_SCALE + expandAnimationProgress_ * (expandedHeight_ - COLLAPSED_HEIGHT*G_SCALE));

    updatePositions();
}

void EntryItem::updatePositions()
{
    messageItem_->setPos(0, titleHeight_ + BODY_MARGIN*G_SCALE);
}

void EntryItem::onExpandRotationAnimationValueChanged(const QVariant &value)
{
    expandAnimationProgress_ = value.toDouble();
    setHeight(COLLAPSED_HEIGHT*G_SCALE + expandAnimationProgress_ * (expandedHeight_ - COLLAPSED_HEIGHT*G_SCALE));
    setIconRotation(180 * expandAnimationProgress_);
    update();

    emit heightChanged(opacityAnimationProgress_ * boundingRect().height());
}

void EntryItem::onOpacityAnimationValueChanged(const QVariant &value)
{
    opacityAnimationProgress_ = value.toDouble();
    iconOpacity_ = OPACITY_SIXTY + (opacityAnimationProgress_ * (1 - OPACITY_SIXTY));
    textOpacity_ = OPACITY_SIXTY + (opacityAnimationProgress_ * (1 - OPACITY_SIXTY));
    icon_->setOpacity(iconOpacity_);
    update();
}

void EntryItem::onClicked()
{
    setExpanded(!expanded_);

    if (expanded_) {
        emit scrollToItem(this);
    }
}

void EntryItem::onHoverEnter()
{
    setAccented(true);
}

void EntryItem::onHoverLeave()
{
    if (!expanded_) {
        setAccented(false);
    }
}

int EntryItem::id() const
{
    return item_.id;
}

void EntryItem::setItem(const api_responses::Notification &item)
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

void EntryItem::setIconRotation(qreal rotation)
{
    QTransform tr;
    tr.translate(0, 8*G_SCALE);
    tr.rotate(rotation, Qt::XAxis);
    tr.translate(0, -8*G_SCALE);
    icon_->setTransform(tr);
}

} // namespace NewsFeedWindow
