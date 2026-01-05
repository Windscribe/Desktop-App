#include "protocollineitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace ProtocolWindow {

ProtocolLineItem::ProtocolLineItem(ScalableGraphicsObject *parent, const types::ProtocolStatus &status, const QString &desc)
    : BaseItem(parent, 64*G_SCALE, "", true, WINDOW_WIDTH - 72), desc_(desc), status_(status), hoverValue_(0.0)
{
    if (status_.status == types::ProtocolStatus::kConnected ||
        status_.status == types::ProtocolStatus::kFailed)
    {
        setClickable(false);
    }
    setAcceptHoverEvents(true);
    connect(&hoverAnimation_, &QVariantAnimation::valueChanged, this, &ProtocolLineItem::onHoverValueChanged);
}

void ProtocolLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    int protocolWidth = 0;

    painter->setRenderHint(QPainter::Antialiasing);

    // outer rounded rectangle
    {
        QPainterPath path;
        path.addRoundedRect(boundingRect().adjusted(kBorderWidth*G_SCALE-1,
                                                    kBorderWidth*G_SCALE-1,
                                                    -kBorderWidth*G_SCALE+1,
                                                    -kBorderWidth*G_SCALE+1),
                            (kRoundedRectRadius+kBorderWidth-1)*G_SCALE, (kRoundedRectRadius+kBorderWidth-1)*G_SCALE);
        QPen pen = painter->pen();
        pen.setWidth(kBorderWidth*G_SCALE);
        pen.setColor(Qt::white);
        QPainterPathStroker stroker(pen);
        painter->setPen(Qt::NoPen);
        if (status_.status == types::ProtocolStatus::Status::kConnected) {
            painter->setOpacity(0.08);
            painter->fillPath(stroker.createStroke(path), QBrush(FontManager::instance().getSeaGreenColor()));
        } else if (status_.status == types::ProtocolStatus::Status::kFailed) {
            painter->setOpacity(0.08);
            painter->fillPath(stroker.createStroke(path), QBrush(QColor(255, 255, 255)));
        } else {
            painter->setOpacity(0.08 + 0.05*hoverValue_);
            painter->fillPath(stroker.createStroke(path), QBrush(QColor(255, 255, 255)));
        }
        painter->setPen(Qt::SolidLine);
    }

    // inner rounded rectangle
    if (status_.status != types::ProtocolStatus::Status::kConnected &&
        status_.status != types::ProtocolStatus::Status::kFailed)
    {
        QPainterPath path;
        painter->setOpacity(1.0);
        path.addRoundedRect(boundingRect().adjusted(kBorderWidth*G_SCALE,
                                                    kBorderWidth*G_SCALE,
                                                    -kBorderWidth*G_SCALE,
                                                    -kBorderWidth*G_SCALE),
                            kRoundedRectRadius*G_SCALE, kRoundedRectRadius*G_SCALE);
        painter->setPen(Qt::NoPen);
        painter->setOpacity(0.08 + 0.05*hoverValue_);
        painter->fillPath(path, QBrush(QColor(255, 255, 255)));
        painter->setPen(Qt::SolidLine);
    }

    // protocol
    {
        if (status_.status == types::ProtocolStatus::Status::kConnected) {
            painter->setOpacity(1.0);
            painter->setPen(FontManager::instance().getSeaGreenColor());
        } else if (status_.status == types::ProtocolStatus::Status::kFailed) {
            painter->setOpacity(OPACITY_SIXTY);
            painter->setPen(QColor(255, 255, 255));
        } else {
            painter->setOpacity(1.0);
            painter->setPen(QColor(255, 255, 255));
        }
        QFont font = FontManager::instance().getFont(14, QFont::Bold);
        QFontMetrics metrics(font);
        protocolWidth = metrics.horizontalAdvance(status_.protocol.toLongString());
        painter->setFont(font);
        painter->drawText(boundingRect().adjusted(kTextIndent*G_SCALE, kFirstLineY*G_SCALE, 0, 0), status_.protocol.toLongString());
    }

    // divider
    {
        painter->setPen(Qt::white);
        painter->setOpacity(0.25);
        painter->drawLine(QPoint(protocolWidth + (kTextIndent + 8)*G_SCALE, kDividerTop*G_SCALE),
                          QPoint(protocolWidth + (kTextIndent + 8)*G_SCALE, kDividerBottom*G_SCALE));
    }

    // port
    {
        QFont font = FontManager::instance().getFont(14,  QFont::Normal);
        painter->setFont(font);

        if (status_.status == types::ProtocolStatus::Status::kConnected) {
            painter->setPen(FontManager::instance().getSeaGreenColor());
        } else {
            painter->setPen(QColor(255, 255, 255));
        }

        painter->setOpacity(OPACITY_SIXTY);
        painter->drawText(boundingRect().adjusted(protocolWidth + (kTextIndent + 17)*G_SCALE, kFirstLineY*G_SCALE, 0, 0), QString::number(status_.port));
    }

    // description
    {
        if (status_.status == types::ProtocolStatus::Status::kConnected) {
            painter->setPen(FontManager::instance().getSeaGreenColor());
        } else if (status_.status == types::ProtocolStatus::Status::kFailed) {
            painter->setPen(QColor(255, 255, 255, 128));
        } else {
            painter->setPen(QColor(255, 255, 255));
        }

        QFont font = FontManager::instance().getFont(14,  QFont::Normal);
        painter->setFont(font);
        painter->setOpacity(OPACITY_SEVENTY);

        int availableWidth = boundingRect().width() - kTextIndent*G_SCALE - 40*G_SCALE;
        QFontMetrics fm(font);
        QString elidedText = desc_;
        if (availableWidth < fm.horizontalAdvance(desc_)) {
            elidedText = fm.elidedText(desc_, Qt::ElideRight, availableWidth, 0);
        }
        painter->drawText(boundingRect().adjusted(kTextIndent*G_SCALE, kSecondLineY*G_SCALE, -40*G_SCALE, 0), elidedText);
    }

    // countdown banner
    if (status_.status == types::ProtocolStatus::Status::kUpNext) {
        // rounded rect
        QPainterPath path;
        path.addRoundedRect(boundingRect().adjusted(boundingRect().width() - 107*G_SCALE, 0, 0, -(boundingRect().height() - 24*G_SCALE)),
                            kRoundedRectRadius*G_SCALE, kRoundedRectRadius*G_SCALE);
        // cover top left corner (not rounded)
        path.addRect(boundingRect().adjusted(boundingRect().width() - 107*G_SCALE, 0, -97*G_SCALE, -(boundingRect().height() - 10*G_SCALE)));
        // cover bottom right corner (not rounded)
        path.addRect(boundingRect().adjusted(boundingRect().width() - 10*G_SCALE, 14*G_SCALE, 0, -(boundingRect().height() - 24*G_SCALE)));

        path.setFillRule(Qt::WindingFill);
        painter->setOpacity(0.1);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QBrush(FontManager::instance().getSeaGreenColor()));
        painter->setPen(Qt::SolidLine);

        // Text inside
        painter->setPen(FontManager::instance().getSeaGreenColor());
        painter->setOpacity(1.0);
        QFont font = FontManager::instance().getFont(14, QFont::Bold);
        painter->setFont(font);
        painter->drawText(boundingRect().adjusted(boundingRect().width() - 107*G_SCALE, 0, 0, -(boundingRect().height() - 24*G_SCALE)),
                          Qt::AlignCenter,
                          QString(tr("NEXT UP IN %1s").arg(QString::number(status_.timeout))));

    // "Connected to" banner
    } else if (status_.status == types::ProtocolStatus::Status::kConnected) {
        // rounded rect
        QPainterPath path;
        path.addRoundedRect(boundingRect().adjusted(boundingRect().width() - 107*G_SCALE,
                                                    kBorderWidth*G_SCALE,
                                                    -kBorderWidth*G_SCALE,
                                                    -(boundingRect().height() - 24*G_SCALE)),
                            kRoundedRectRadius*G_SCALE, kRoundedRectRadius*G_SCALE);
        // cover top left corner (not rounded)
        path.addRect(boundingRect().adjusted(boundingRect().width() - 107*G_SCALE,
                                             kBorderWidth*G_SCALE,
                                             -97*G_SCALE,
                                             -(boundingRect().height() - 10*G_SCALE)));
        // cover bottom right corner (not rounded)
        path.addRect(boundingRect().adjusted(boundingRect().width() - 10*G_SCALE,
                                             14*G_SCALE,
                                             -kBorderWidth*G_SCALE,
                                             -(boundingRect().height() - 24*G_SCALE)));

        path.setFillRule(Qt::WindingFill);
        painter->setOpacity(0.1);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QBrush(FontManager::instance().getSeaGreenColor()));
        painter->setPen(Qt::SolidLine);

        painter->setPen(FontManager::instance().getSeaGreenColor());
        painter->setOpacity(1.0);
        QFont font = FontManager::instance().getFont(14,  QFont::Normal);
        painter->setFont(font);
        painter->drawText(boundingRect().adjusted(boundingRect().width() - 107*G_SCALE, 0, 0, -(boundingRect().height() - 24*G_SCALE)),
                          Qt::AlignCenter,
                          QString(tr("Connected to")));
    }

    // Checkmark
    if (status_.status == types::ProtocolStatus::Status::kConnected) {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("CHECKMARK");
        painter->setOpacity(1.0);
        p->draw(boundingRect().width() - 16*G_SCALE - p->width(), 36*G_SCALE, painter);
    } else if (status_.status == types::ProtocolStatus::Status::kFailed) {
        painter->setOpacity(1.0);
        QFont font = FontManager::instance().getFont(14,  QFont::Normal);
        painter->setFont(font);
        painter->setPen(QColor(249, 76, 67));
        painter->drawText(boundingRect().adjusted(0, 0, -16*G_SCALE, 0),
                          Qt::AlignRight | Qt::AlignVCenter,
                          QString(tr("Failed")));
    } else {
        painter->setOpacity(OPACITY_SIXTY + 0.4*hoverValue_);
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/FRWRD_ARROW_WHITE_ICON");
        if (status_.status == types::ProtocolStatus::Status::kUpNext) {
            p->draw(boundingRect().width() - 16*G_SCALE - p->width(), 36*G_SCALE, painter);
        } else {
            p->draw(boundingRect().width() - 16*G_SCALE - p->width(), 24*G_SCALE, painter);
        }
    }
}

types::Protocol ProtocolLineItem::protocol()
{
    return status_.protocol;
}

uint ProtocolLineItem::port()
{
    return status_.port;
}

types::ProtocolStatus::Status ProtocolLineItem::status()
{
    return status_.status;
}

int ProtocolLineItem::decrementCountdown()
{
    status_.timeout--;
    update();
    return status_.timeout;
}

void ProtocolLineItem::clearCountdown()
{
    if (status_.status == types::ProtocolStatus::Status::kUpNext) {
        status_.status = types::ProtocolStatus::Status::kDisconnected;
        status_.timeout = -1;
    }
    update();
}

void ProtocolLineItem::updateScaling() {
    setHeight(64*G_SCALE);
    update();
}

void ProtocolLineItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    startAnAnimation<double>(hoverAnimation_, hoverValue_, 1.0, ANIMATION_SPEED_FAST);
}

void ProtocolLineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    startAnAnimation<double>(hoverAnimation_, hoverValue_, 0.0, ANIMATION_SPEED_FAST);
}

void ProtocolLineItem::onHoverValueChanged(const QVariant &value)
{
    hoverValue_ = value.toDouble();
    update();
}

void ProtocolLineItem::setDescription(const QString &desc)
{
    desc_ = desc;
    update();
}

} // namespace ProtocolWindow
