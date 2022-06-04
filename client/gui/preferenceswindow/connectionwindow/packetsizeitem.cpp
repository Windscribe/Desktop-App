#include "packetsizeitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "../dividerline.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"

#include <QDebug>

namespace PreferencesWindow {

PacketSizeItem::PacketSizeItem(ScalableGraphicsObject *parent)
    : BaseItem(parent, 50), isExpanded_(false), isShowingError_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    switchItem_ = new AutoManualSwitchItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::AutoManualSwitchItem", "Packet Size"));
    connect(switchItem_, SIGNAL(stateChanged(AutoManualSwitchItem::SWITCH_STATE)), SLOT(onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE)));
    switchItem_->setPos(0, 0);

    editBoxPacketSize_ = new PacketSizeEditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "MTU"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter MTU"), false, "preferences/AUTODETECT_ICON");
    connect(editBoxPacketSize_, SIGNAL(textChanged(QString)), SLOT(onEditBoxTextChanged(QString)));
    connect(editBoxPacketSize_, SIGNAL(additionalButtonHoverEnter()), SLOT(onAutoDetectAndGenerateHoverEnter()));
    connect(editBoxPacketSize_, SIGNAL(additionalButtonHoverLeave()), SLOT(onAutoDetectAndGenerateHoverLeave()));
    connect(editBoxPacketSize_, SIGNAL(additionalButtonClicked()), SIGNAL(detectAppropriatePacketSizeButtonClicked()));

    editBoxPacketSize_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    setHeight(EXPANDED_HEIGHT);

    QRegularExpression ipRegex ("^\\d+$");
    QRegularExpressionValidator *integerValidator = new QRegularExpressionValidator(ipRegex, this);
    editBoxPacketSize_->setValidator(integerValidator);

    expandEnimation_.setStartValue(COLLAPSED_HEIGHT*G_SCALE);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT*G_SCALE);
    expandEnimation_.setDuration(150);
    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
}

void PacketSizeItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt());
}

void PacketSizeItem::onAutoDetectAndGenerateHoverEnter()
{
    if (isShowingError_)
        return;

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(editBoxPacketSize_->scenePos()));

    int posX = globalPt.x() + 226 * G_SCALE;
    int posY = globalPt.y() + 8 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);
    ti.x = posX;
    ti.y = posY;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.8;
    ti.title = tr("Auto-Detect & Generate MTU");
    TooltipController::instance().showTooltipBasic(ti);
}

void PacketSizeItem::onAutoDetectAndGenerateHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);
    if (isShowingError_) {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE_ERROR);
        isShowingError_ = false;
    }
}

void PacketSizeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void PacketSizeItem::setPacketSize(const ProtoTypes::PacketSize &ps)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(curPacketSize_, ps))
    {
        curPacketSize_ = ps;

        if (curPacketSize_.is_automatic())
        {
            switchItem_->setState(AutoManualSwitchItem::AUTO);
            isExpanded_ = false;
            setHeight(COLLAPSED_HEIGHT*G_SCALE);
        }
        else
        {

            switchItem_->setState(AutoManualSwitchItem::MANUAL);
            isExpanded_ = true;
            setHeight(EXPANDED_HEIGHT*G_SCALE);
        }

        editBoxPacketSize_->setEditButtonClickable(!curPacketSize_.is_automatic());
        if (curPacketSize_.mtu() >= 0)
        {
            editBoxPacketSize_->setText(QString::number(curPacketSize_.mtu()));
        }
        else
        {
            editBoxPacketSize_->setText("");
        }
    }
}

void PacketSizeItem::setPacketSizeDetectionState(bool on)
{
    editBoxPacketSize_->setAdditionalButtonBusyState(on);
}

void PacketSizeItem::showPacketSizeDetectionError(const QString &title, const QString &message)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(editBoxPacketSize_->scenePos()));

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE_ERROR);
    ti.x = globalPt.x() + 226 * G_SCALE;
    ti.y = globalPt.y() + 8 * G_SCALE;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.8;
    ti.title = title;
    ti.desc = message;
    ti.width = 150 * G_SCALE;
    ti.delay = 100;

    QTimer::singleShot(0, [this, ti]() {
        isShowingError_ = true;
        editBoxPacketSize_->setAdditionalButtonSelectedState(true);
        TooltipController::instance().showTooltipBasic(ti);
    });
}

void PacketSizeItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(isExpanded_? EXPANDED_HEIGHT*G_SCALE : COLLAPSED_HEIGHT*G_SCALE);
    editBoxPacketSize_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    expandEnimation_.setStartValue(COLLAPSED_HEIGHT*G_SCALE);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT*G_SCALE);
}

bool PacketSizeItem::hasItemWithFocus()
{
    return editBoxPacketSize_->lineEditHasFocus();
}

void PacketSizeItem::onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state)
{
    if (state == AutoManualSwitchItem::MANUAL /*&& !isExpanded_*/)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = true;

        curPacketSize_.set_is_automatic(false);
        emit packetSizeChanged(curPacketSize_);
    }
    else if (isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = false;

        curPacketSize_.set_is_automatic(true);
        emit packetSizeChanged(curPacketSize_);
    }
    editBoxPacketSize_->setEditButtonClickable(!curPacketSize_.is_automatic());
}

void PacketSizeItem::onEditBoxTextChanged(const QString &text)
{
    if (text.isEmpty())
    {
        curPacketSize_.set_mtu(-1);
    }
    else
    {
        curPacketSize_.set_mtu(text.toInt());
    }
    emit packetSizeChanged(curPacketSize_);
}

} // namespace PreferencesWindow
