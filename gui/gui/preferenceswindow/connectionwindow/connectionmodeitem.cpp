#include "connectionmodeitem.h"

#include <QPainter>
#include "../dividerline.h"
#include "graphicresources/fontmanager.h"
#include "utils/protoenumtostring.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ConnectionModeItem::ConnectionModeItem(ScalableGraphicsObject *parent, PreferencesHelper *preferencesHelper) : BaseItem(parent, 50),
    preferencesHelper_(preferencesHelper),
    isPortMapInitialized_(false),
    isExpanded_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

#if defined(Q_OS_LINUX)
    curConnectionMode_.set_protocol(ProtoTypes::Protocol::PROTOCOL_UDP);
    curConnectionMode_.set_port(443);
#endif

    connect(dynamic_cast<QObject*>(preferencesHelper), SIGNAL(portMapChanged()), SLOT(onPortMapChanged()));

    // default connection mode
    //curConnectionMode_.set_is_automatic(true);
    //curConnectionMode_.set_protocol(ProtoTypes::PROTOCOL_IKEV2);
    //curConnectionMode_.set_port(500);

    switchItem_ = new AutoManualSwitchItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::AutoManualSwitchItem", "Connection Mode"));
    connect(switchItem_, SIGNAL(stateChanged(AutoManualSwitchItem::SWITCH_STATE)), SLOT(onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE)));
    switchItem_->setPos(0, 0);

    //const QVector<ProtoTypes::Protocol> protocols = preferencesHelper->getAvailableProtocols();
    comboBoxProtocol_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Protocol"), "", 43, QColor(16, 22, 40), 24, true);
    comboBoxProtocol_->setPos(0, COLLAPSED_HEIGHT);
    comboBoxProtocol_->setClickable(false);
    connect(comboBoxProtocol_, &ComboBoxItem::buttonHoverEnter,
        [this]() { emit buttonHoverEnter(ButtonType::PROTOCOL); });
    connect(comboBoxProtocol_, &ComboBoxItem::buttonHoverLeave,
        [this]() { emit buttonHoverLeave(ButtonType::PROTOCOL); });

    /*for (auto pd : protocols)
    {
        comboBoxProtocol_->addItem(ProtoEnumToString::instance().toString(pd), (int)pd);
    }
    if (protocols.size() > 0)
    {
        comboBoxProtocol_->setCurrentItem((int)*protocols.begin());
    }
    else
    {
        comboBoxProtocol_->setClickable(false);
    }*/

    connect(comboBoxProtocol_, SIGNAL(currentItemChanged(QVariant)), SLOT(onCurrentProtocolItemChanged(QVariant)));


    comboBoxPort_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Port"), "", 43, QColor(16, 22, 40), 24, true);
    comboBoxPort_->setPos(0, COLLAPSED_HEIGHT + 43);
    comboBoxPort_->setClickable(false);

    connect(comboBoxPort_, SIGNAL(currentItemChanged(QVariant)), SLOT(onCurrentPortItemChanged(QVariant)));
    connect(comboBoxPort_, &ComboBoxItem::buttonHoverEnter,
        [this]() { emit buttonHoverEnter(ButtonType::PORT); });
    connect(comboBoxPort_, &ComboBoxItem::buttonHoverLeave,
        [this]() { emit buttonHoverLeave(ButtonType::PORT); });

    /*if (protocols.size() > 0)
    {
        updateProtocol(*protocols.begin());
    }
    else
    {
        comboBoxPort_->clear();
        comboBoxPort_->setClickable(false);
    }*/

    expandEnimation_.setStartValue(COLLAPSED_HEIGHT); // scaled in slot
    expandEnimation_.setEndValue(EXPANDED_HEIGHT);    // scaled in slot
    expandEnimation_.setDuration(150);
    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
}

void ConnectionModeItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt()*G_SCALE);
}

void ConnectionModeItem::onCurrentProtocolItemChanged(QVariant value)
{
    updateProtocol((ProtoTypes::Protocol)value.toInt());

    curConnectionMode_.set_protocol((ProtoTypes::Protocol)comboBoxProtocol_->currentItem().toInt());
    curConnectionMode_.set_port(comboBoxPort_->currentItem().toInt());

    emit connectionlModeChanged(curConnectionMode_);
}

void ConnectionModeItem::onCurrentPortItemChanged(QVariant value)
{
    curConnectionMode_.set_port(value.toInt());
    emit connectionlModeChanged(curConnectionMode_);
}

void ConnectionModeItem::onPortMapChanged()
{
    comboBoxProtocol_->clear();
    comboBoxPort_->clear();

    const QVector<ProtoTypes::Protocol> protocols = preferencesHelper_->getAvailableProtocols();
    if (protocols.size() > 0)
    {
        isPortMapInitialized_ = true;
        for (auto pd : protocols)
        {
            comboBoxProtocol_->addItem(ProtoEnumToString::instance().toString(pd), (int)pd);
        }
        comboBoxProtocol_->setCurrentItem((int)*protocols.begin());
        comboBoxProtocol_->setMaxMenuItemsShowing(protocols.count());
        updateProtocol(*protocols.begin());
        comboBoxProtocol_->setClickable(true);
        comboBoxPort_->setClickable(true);
    }
    else
    {
        isPortMapInitialized_ = false;
        comboBoxProtocol_->setClickable(false);
        comboBoxPort_->setClickable(false);
    }
    updateConnectionMode();
}

void ConnectionModeItem::hideOpenPopups()
{
    comboBoxProtocol_->hideMenu();
    comboBoxPort_    ->hideMenu();
}

void ConnectionModeItem::updateProtocol(ProtoTypes::Protocol protocol)
{
    comboBoxPort_->clear();
    const QVector<uint> ports = preferencesHelper_->getAvailablePortsForProtocol(protocol);
    for (uint p : ports)
    {
        comboBoxPort_->addItem(QString::number(p), p);
    }
    comboBoxPort_->setCurrentItem(*ports.begin());
}

void ConnectionModeItem::updateConnectionMode()
{
    if (isPortMapInitialized_)
    {
        if (curConnectionMode_.is_automatic())
        {
            switchItem_->setState(AutoManualSwitchItem::AUTO);
            isExpanded_ = false;
            setHeight(COLLAPSED_HEIGHT * G_SCALE);
        }
        else
        {
            comboBoxProtocol_->setCurrentItem((int)curConnectionMode_.protocol());
            updateProtocol(curConnectionMode_.protocol());
            comboBoxPort_->setCurrentItem(curConnectionMode_.port());

            switchItem_->setState(AutoManualSwitchItem::MANUAL);
            isExpanded_ = true;

            setHeight(EXPANDED_HEIGHT * G_SCALE);
        }
    }
}

void ConnectionModeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void ConnectionModeItem::setConnectionMode(const ProtoTypes::ConnectionSettings &cm)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(curConnectionMode_, cm))
    {
        curConnectionMode_ = cm;
        updateConnectionMode();
    }
}

QPointF ConnectionModeItem::getButtonScenePos(ButtonType type) const
{
    QPointF result;
    switch (type) {
    case ButtonType::PROTOCOL:
        result = comboBoxProtocol_->getButtonScenePos();
        break;
    case ButtonType::PORT:
        result = comboBoxPort_->getButtonScenePos();
        break;
    }
    return result;
}

void ConnectionModeItem::updateScaling()
{
    BaseItem::updateScaling();

    setHeight(isExpanded_ ? EXPANDED_HEIGHT*G_SCALE : COLLAPSED_HEIGHT*G_SCALE);
    comboBoxPort_->setPos(0, (COLLAPSED_HEIGHT + 43)*G_SCALE);
    comboBoxProtocol_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
}

void ConnectionModeItem::onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state)
{
    if (state == AutoManualSwitchItem::MANUAL && !isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = true;

        curConnectionMode_.set_is_automatic(false);
        emit connectionlModeChanged(curConnectionMode_);
    }
    else if (isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = false;

        curConnectionMode_.set_is_automatic(true);
        emit connectionlModeChanged(curConnectionMode_);
    }
}

void ConnectionModeItem::onFirewallWhenChanged(QVariant v)
{
    Q_UNUSED(v);
}


} // namespace PreferencesWindow
