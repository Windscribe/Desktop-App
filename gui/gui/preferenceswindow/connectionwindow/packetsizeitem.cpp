#include "packetsizeitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <google/protobuf/util/message_differencer.h>
#include "../dividerline.h"
#include "GraphicResources/fontmanager.h"
#include "CommonGraphics/commongraphics.h"
#include "dpiscalemanager.h"

#include <QDebug>

namespace PreferencesWindow {

PacketSizeItem::PacketSizeItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
  // , isExpanded_(true)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    switchItem_ = new AutoManualSwitchItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::AutoManualSwitchItem", "Packet Size"));
    connect(switchItem_, SIGNAL(stateChanged(AutoManualSwitchItem::SWITCH_STATE)), SLOT(onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE)));
    switchItem_->setPos(0, 0);

    editBoxMSS_ = new MssEditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "MSS"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter MSS"), false, "preferences/AUTODETECT_ICON");
    connect(editBoxMSS_, SIGNAL(textChanged(QString)), SLOT(onMssChanged(QString)));
    connect(editBoxMSS_, SIGNAL(additionalButtonHoverEnter()), SLOT(onAutoDetectAndGenerateHoverEnter()));
    connect(editBoxMSS_, SIGNAL(additionalButtonHoverLeave()), SLOT(onAutoDetectAndGenerateHoverLeave()));
    connect(editBoxMSS_, SIGNAL(additionalButtonClicked()), SIGNAL(detectPacketMssButtonClicked()));

    editBoxMSS_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    setHeight(EXPANDED_HEIGHT);

    QRegExp ipRegex ("^\\d+$");
    QRegExpValidator *integerValidator = new QRegExpValidator(ipRegex, this);
    editBoxMSS_->setValidator(integerValidator);

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
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(editBoxMSS_->scenePos()));

    int posX = globalPt.x() + 226 * G_SCALE;
    int posY = globalPt.y() + 8 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);
    ti.x = posX;
    ti.y = posY;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.8;
    ti.title = tr("Auto-Detect & Generate MSS");
    emit showTooltip(ti);
}

void PacketSizeItem::onAutoDetectAndGenerateHoverLeave()
{
    emit hideTooltip(TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);
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

        editBoxMSS_->setEditButtonClickable(!curPacketSize_.is_automatic());
        if (curPacketSize_.mss() >= 0)
        {
            editBoxMSS_->setText(QString::number(curPacketSize_.mss()));
        }
        else
        {
            editBoxMSS_->setText("");
        }
    }
}

void PacketSizeItem::setPacketSizeDetectionState(bool on)
{
    editBoxMSS_->setAdditionalButtonBusyState(on);
}

void PacketSizeItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(isExpanded_? EXPANDED_HEIGHT*G_SCALE : COLLAPSED_HEIGHT*G_SCALE);
    editBoxMSS_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    expandEnimation_.setStartValue(COLLAPSED_HEIGHT*G_SCALE);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT*G_SCALE);
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
    editBoxMSS_->setEditButtonClickable(!curPacketSize_.is_automatic());
}

void PacketSizeItem::onMssChanged(const QString &text)
{
    if (text.isEmpty())
    {
        curPacketSize_.set_mss(-1);
    }
    else
    {
        curPacketSize_.set_mss(text.toInt());
    }
    emit packetSizeChanged(curPacketSize_);
}

} // namespace PreferencesWindow
