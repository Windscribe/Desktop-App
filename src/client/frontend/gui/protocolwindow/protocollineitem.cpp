#include "protocollineitem.h"
#include "protocolpromptitem.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace ProtocolWindow {

ProtocolLineItem::ProtocolLineItem(ScalableGraphicsObject *parent, const types::ProtocolStatus &status, const QString &desc, const QVector<uint> &availablePorts)
    : BaseItem(parent, 64*G_SCALE, "", true, WINDOW_WIDTH - 72), desc_(desc), status_(status), availablePorts_(availablePorts), hoverValue_(0.0), portAreaHovered_(false), portMenu_(nullptr), portMenuProxy_(nullptr)
{
    if (status_.status == types::ProtocolStatus::kConnected ||
        status_.status == types::ProtocolStatus::kFailed)
    {
        setClickable(false);
    }
    setAcceptHoverEvents(true);
    connect(&hoverAnimation_, &QVariantAnimation::valueChanged, this, &ProtocolLineItem::onHoverValueChanged);

    bool showPortDropdown = false;
    ProtocolPromptItem *promptItem = qobject_cast<ProtocolPromptItem*>(parent);
    if (promptItem && promptItem->mode() == ProtocolWindowMode::kChangeProtocol) {
        showPortDropdown = true;
    }

    if (showPortDropdown && availablePorts_.size() > 1 && status_.status == types::ProtocolStatus::kDisconnected) {
        portDropdownIcon_ = new IconButton(16, 16, "preferences/CNTXT_MENU_ICON", "", this);
        portDropdownIcon_->setClickable(false);
        portDropdownIcon_->setUnhoverOpacity(0.7);
        portDropdownIcon_->setHoverOpacity(OPACITY_FULL);
        updatePortDropdownIconPosition();
    } else {
        portDropdownIcon_ = nullptr;
    }
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

        painter->setOpacity(portAreaHovered_ ? OPACITY_FULL : OPACITY_SIXTY);
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

        int rightMargin = 40*G_SCALE;
        if (status_.status == types::ProtocolStatus::Status::kFailed) {
            rightMargin += 24*G_SCALE;
        }
        int availableWidth = boundingRect().width() - kTextIndent*G_SCALE - rightMargin;
        QFontMetrics fm(font);
        QString elidedText = desc_;
        if (availableWidth < fm.horizontalAdvance(desc_)) {
            elidedText = fm.elidedText(desc_, Qt::ElideRight, availableWidth, 0);
        }
        painter->drawText(boundingRect().adjusted(kTextIndent*G_SCALE, kSecondLineY*G_SCALE, -rightMargin, 0), elidedText);
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
    updatePortDropdownIconPosition();
    update();
}

void ProtocolLineItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    startAnAnimation<double>(hoverAnimation_, hoverValue_, 1.0, ANIMATION_SPEED_FAST);
    updatePortHoverState(event->pos());
}

void ProtocolLineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    startAnAnimation<double>(hoverAnimation_, hoverValue_, 0.0, ANIMATION_SPEED_FAST);
    portAreaHovered_ = false;
    if (portDropdownIcon_) {
        portDropdownIcon_->unhover();
    }
    update();
}

void ProtocolLineItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    updatePortHoverState(event->pos());
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

void ProtocolLineItem::updatePortDropdownIconPosition()
{
    if (!portDropdownIcon_) return;

    QFont font = FontManager::instance().getFont(14, QFont::Bold);
    QFontMetrics fm(font);
    const QString protocolString = status_.protocol.toLongString();
    const int protocolWidth = fm.horizontalAdvance(protocolString);

    QFont portFont = FontManager::instance().getFont(14, QFont::Normal);
    QFontMetrics portFm(portFont);
    const int portWidth = portFm.horizontalAdvance(QString::number(status_.port));

    const int portX = protocolWidth + (kTextIndent + 17)*G_SCALE;
    const int iconX = portX + portWidth + 4*G_SCALE;

    portDropdownIcon_->setPos(iconX, kFirstLineY*G_SCALE);
}

void ProtocolLineItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (availablePorts_.size() > 1 && status_.status == types::ProtocolStatus::kDisconnected && isPortAreaHovered(event->pos())) {
        ProtocolPromptItem *promptItem = dynamic_cast<ProtocolPromptItem*>(parentItem());
        if (promptItem) {
            promptItem->hideOpenPopups();
        }
        showPortMenu();
        event->accept();
    } else {
        BaseItem::mousePressEvent(event);
    }
}

bool ProtocolLineItem::isPortAreaHovered(const QPointF &pos) const
{
    if (!portDropdownIcon_) return false;

    QFont font = FontManager::instance().getFont(14, QFont::Bold);
    QFontMetrics fm(font);
    const QString protocolString = status_.protocol.toLongString();
    const int protocolWidth = fm.horizontalAdvance(protocolString);

    const int portX = protocolWidth + (kTextIndent + 17)*G_SCALE;
    QFont portFont = FontManager::instance().getFont(14, QFont::Normal);
    QFontMetrics portFm(portFont);
    const int portWidth = portFm.horizontalAdvance(QString::number(status_.port));

    QRectF portRect(portX, kFirstLineY*G_SCALE - 5*G_SCALE, portWidth + portDropdownIcon_->boundingRect().width() + 10*G_SCALE, 20*G_SCALE);
    return portRect.contains(pos);
}

void ProtocolLineItem::showPortMenu()
{
    if (!portMenu_) {
        portMenu_ = new CommonWidgets::ComboMenuWidget();
        portMenu_->setMaxItemsShowing(5);
        for (uint port : availablePorts_) {
            portMenu_->addItem(QString::number(port), QVariant(port));
        }
        connect(portMenu_, &CommonWidgets::ComboMenuWidget::itemClicked, this, &ProtocolLineItem::onPortMenuItemSelected);
        connect(portMenu_, &CommonWidgets::ComboMenuWidget::hidden, this, &ProtocolLineItem::onPortMenuHidden);

        portMenuProxy_ = new QGraphicsProxyWidget(parentItem());
        portMenuProxy_->setWidget(portMenu_);
        portMenuProxy_->setZValue(1000);
        portMenuProxy_->setFlag(QGraphicsItem::ItemIsPanel, true);
    }

    QFont font = FontManager::instance().getFont(14, QFont::Bold);
    QFontMetrics fm(font);
    const QString protocolString = status_.protocol.toLongString();
    const int protocolWidth = fm.horizontalAdvance(protocolString);

    QFont portFont = FontManager::instance().getFont(14, QFont::Normal);
    QFontMetrics portFm(portFont);
    const int portWidth = portFm.horizontalAdvance(QString::number(status_.port));

    const int portX = protocolWidth + (kTextIndent + 17)*G_SCALE;
    const int rightEdge = portX + portWidth + portDropdownIcon_->boundingRect().width() + 4*G_SCALE;
    const int menuX = rightEdge - portMenu_->width();

    QPointF globalPos = mapToParent(QPointF(menuX, kFirstLineY*G_SCALE));
    portMenuProxy_->setPos(globalPos);
    portMenuProxy_->setVisible(true);
    portMenu_->show();
    portMenu_->setFocus();
}

void ProtocolLineItem::onPortMenuItemSelected(QString text, QVariant data)
{
    status_.port = data.toUInt();
    portMenu_->hide();
    updatePortDropdownIconPosition();
    update();
}

void ProtocolLineItem::onPortMenuHidden()
{
    if (portMenuProxy_) {
        portMenuProxy_->setVisible(false);
    }
}

void ProtocolLineItem::hideOpenPopups()
{
    if (portMenuProxy_) {
        portMenuProxy_->setVisible(false);
    }
}

void ProtocolLineItem::updatePortHoverState(const QPointF &pos)
{
    bool wasHovered = portAreaHovered_;
    portAreaHovered_ = isPortAreaHovered(pos);

    if (portDropdownIcon_) {
        if (portAreaHovered_) {
            portDropdownIcon_->hover();
        } else {
            portDropdownIcon_->unhover();
        }
    }

    if (wasHovered != portAreaHovered_) {
        update();
    }
}

} // namespace ProtocolWindow
