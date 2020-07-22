#include "macspoofingitem.h"

#include <QPainter>
#include <QMessageBox>
#include "GraphicResources/fontmanager.h"
#include <google/protobuf/util/message_differencer.h>
#include "Utils/logger.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

MacSpoofingItem::MacSpoofingItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    checkBoxEnable_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "MAC Spoofing"), "");
    connect(checkBoxEnable_, SIGNAL(stateChanged(bool)), SLOT(onCheckBoxStateChanged(bool)));
    checkBoxEnable_->setPos(0, 0);

    macAddressItem_ = new MacAddressItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::MacAddressItem", "MAC Address"));
    connect(macAddressItem_, SIGNAL(cycleMacAddressClick()), SLOT(onCycleMacAddressClick()));

    autoRotateMacItem_ = new AutoRotateMacItem(this);
    connect(autoRotateMacItem_, SIGNAL(stateChanged(bool)), SLOT(onAutoRotateMacStateChanged(bool)));

    comboBoxInterface_ = new ComboBoxItem(this, tr("Interface"), "", 43, QColor(16, 22, 40), 24, true);
    connect(comboBoxInterface_, SIGNAL(currentItemChanged(QVariant)), SLOT(onInterfaceItemChanged(QVariant)));

    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
    expandEnimation_.setStartValue(COLLAPSED_HEIGHT);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT);
    expandEnimation_.setDuration(150);

    updatePositions();
}

void MacSpoofingItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRect topHalfRect(0, 0, boundingRect().width(), 50);
    QFont *font = FontManager::instance().getFont(13, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));

   // painter->drawText(topHalfRect.adjusted(16, 0, 0, 0), Qt::AlignVCenter, tr("Proxy Gateway"));

    //QRect bottomHalfRect(16, 45, boundingRect().width() - 40 - checkBoxButton_->boundingRect().width(), 40);
    //painter->fillRect(bottomHalfRect, QBrush(QColor(255, 0 , 0)));
}

void MacSpoofingItem::setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(curMAS_, macAddrSpoofing))
    {
        curMAS_ = macAddrSpoofing;
        updateSpoofingSelection(curMAS_);

        if (macAddrSpoofing.is_enabled())
        {
            checkBoxEnable_->setState(true);
            setHeight(EXPANDED_HEIGHT*G_SCALE);
            macAddressItem_->setMacAddress(QString::fromStdString(macAddrSpoofing.mac_address()));
        }
        else
        {
            checkBoxEnable_->setState(false);
            setHeight(COLLAPSED_HEIGHT*G_SCALE);
            macAddressItem_->setMacAddress(QString::fromStdString(macAddrSpoofing.mac_address()));
        }

        if (macAddrSpoofing.is_auto_rotate())
        {
            autoRotateMacItem_->setState(true);
        }
        else
        {
            autoRotateMacItem_->setState(false);
        }
    }
}

void MacSpoofingItem::setCurrentNetwork(const ProtoTypes::NetworkInterface &networkInterface)
{
    curNetworkInterface_ = networkInterface;
}

void MacSpoofingItem::updateScaling()
{
    BaseItem::updateScaling();

    setHeight(checkBoxEnable_->isChecked() ? EXPANDED_HEIGHT*G_SCALE : COLLAPSED_HEIGHT*G_SCALE);
    updatePositions();
}

void MacSpoofingItem::onCheckBoxStateChanged(bool isChecked)
{
    if (isChecked)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }

        curMAS_.set_is_enabled(true);
        qCDebug(LOG_USER) << "Enabled MAC Spoofing";
        emit macAddrSpoofingChanged(curMAS_);
    }
    else
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        curMAS_.set_is_enabled(false);
        qCDebug(LOG_USER) << "Disabled MAC Spoofing";
        emit macAddrSpoofingChanged(curMAS_);
    }
}

void MacSpoofingItem::onAutoRotateMacStateChanged(bool isChecked)
{
    qCDebug(LOG_USER) << "Changed Auto-Rotate: " << isChecked;
    curMAS_.set_is_auto_rotate(isChecked);
    emit macAddrSpoofingChanged(curMAS_);
}

void MacSpoofingItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt()*G_SCALE);
}

void MacSpoofingItem::onInterfaceItemChanged(const QVariant &value)
{
    int interfaceIndex = value.toInt();

    ProtoTypes::MacAddrSpoofing newMAS;

    for (int i = 0; i < curMAS_.network_interfaces().networks_size(); i++)
    {
        if (curMAS_.network_interfaces().networks(i).interface_index() == interfaceIndex)
        {
            *curMAS_.mutable_selected_network_interface() = curMAS_.network_interfaces().networks(i);
            break;
        }
    }

    qCDebug(LOG_USER) << "Changed MAC Spoof Interface Selection: " << QString::fromStdString(curMAS_.selected_network_interface().interface_name());
    emit macAddrSpoofingChanged(curMAS_);
}

void MacSpoofingItem::onCycleMacAddressClick()
{
    if (curNetworkInterface_.interface_index() == curMAS_.selected_network_interface().interface_index())
    {
        if (curNetworkInterface_.interface_index() != -1)
        {
            emit cycleMacAddressClick();
        }
        else
        {
            qCDebug(LOG_BASIC) << "Cannot spoof on 'No Interface'";
            QMessageBox::information(nullptr, QString(tr("Cannot spooof on 'No Interface'")), QString(tr("You can only spoof an existing adapter.")));
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "Cannot spoof the current interface -- must match selected interface";
        QMessageBox::information(nullptr, QString(tr("Cannot spooof the current interface")), QString(tr("The current primary interface must match the selected interface to spoof.")));
    }
}

void MacSpoofingItem::updateSpoofingSelection(ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    QString adapters = "";
    comboBoxInterface_->clear();
    for (int i = 0; i < macAddrSpoofing.network_interfaces().networks_size(); i++)
    {
        ProtoTypes::NetworkInterface interface = macAddrSpoofing.network_interfaces().networks(i);
        comboBoxInterface_->addItem(QString::fromStdString(interface.interface_name()), interface.interface_index());

        adapters += QString::fromStdString(interface.interface_name());

        if (i < macAddrSpoofing.network_interfaces().networks_size() - 1) adapters += ", ";
    }

    qCDebug(LOG_BASIC) << "Updating Spoofing adapter dropdown with: " << adapters;
    qCDebug(LOG_BASIC) << "Spoofing selection: " << QString::fromStdString(macAddrSpoofing.selected_network_interface().interface_name());

    comboBoxInterface_->setCurrentItem(macAddrSpoofing.selected_network_interface().interface_index());
    *macAddrSpoofing.mutable_selected_network_interface() = macAddrSpoofing.selected_network_interface();

}

void MacSpoofingItem::updatePositions()
{
    macAddressItem_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    autoRotateMacItem_->setPos(0, (COLLAPSED_HEIGHT + 43)*G_SCALE);
    comboBoxInterface_->setPos(0, (COLLAPSED_HEIGHT + 43 + 43)*G_SCALE);
}

} // namespace PreferencesWindow
