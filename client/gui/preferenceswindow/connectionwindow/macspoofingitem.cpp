#include "macspoofingitem.h"

#include <QPainter>
#include <QMessageBox>
#include "graphicresources/fontmanager.h"
#include "utils/logger.h"
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

void MacSpoofingItem::setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if(curMAS_ != macAddrSpoofing)
    {
        curMAS_ = macAddrSpoofing;
        updateSpoofingSelection(curMAS_);

        if (macAddrSpoofing.isEnabled)
        {
            checkBoxEnable_->setState(true);
            setHeight(EXPANDED_HEIGHT*G_SCALE);
            macAddressItem_->setMacAddress(macAddrSpoofing.macAddress);
        }
        else
        {
            checkBoxEnable_->setState(false);
            setHeight(COLLAPSED_HEIGHT*G_SCALE);
            macAddressItem_->setMacAddress(macAddrSpoofing.macAddress);
        }

        if (macAddrSpoofing.isAutoRotate)
        {
            autoRotateMacItem_->setState(true);
        }
        else
        {
            autoRotateMacItem_->setState(false);
        }
    }
}

void MacSpoofingItem::setCurrentNetwork(const types::NetworkInterface &networkInterface)
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

        curMAS_.isEnabled = true;
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
        curMAS_.isEnabled = false;
        qCDebug(LOG_USER) << "Disabled MAC Spoofing";
        emit macAddrSpoofingChanged(curMAS_);
    }
}

void MacSpoofingItem::onAutoRotateMacStateChanged(bool isChecked)
{
    qCDebug(LOG_USER) << "Changed Auto-Rotate: " << isChecked;
    curMAS_.isAutoRotate = isChecked;
    emit macAddrSpoofingChanged(curMAS_);
}

void MacSpoofingItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt()*G_SCALE);
}

void MacSpoofingItem::onInterfaceItemChanged(const QVariant &value)
{
    int interfaceIndex = value.toInt();

    for (int i = 0; i < curMAS_.networkInterfaces.size(); i++)
    {
        if (curMAS_.networkInterfaces[i].interfaceIndex == interfaceIndex)
        {
            curMAS_.selectedNetworkInterface = curMAS_.networkInterfaces[i];
            break;
        }
    }

    qCDebug(LOG_USER) << "Changed MAC Spoof Interface Selection: " << curMAS_.selectedNetworkInterface.interfaceName;
    emit macAddrSpoofingChanged(curMAS_);
}

void MacSpoofingItem::onCycleMacAddressClick()
{
    if (curNetworkInterface_.interfaceIndex == curMAS_.selectedNetworkInterface.interfaceIndex)
    {
        if (curNetworkInterface_.interfaceIndex != -1)
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

void MacSpoofingItem::updateSpoofingSelection(types::MacAddrSpoofing &macAddrSpoofing)
{
    QString adapters = "";
    comboBoxInterface_->clear();
    for (int i = 0; i < macAddrSpoofing.networkInterfaces.size(); i++)
    {
        types::NetworkInterface interface = macAddrSpoofing.networkInterfaces[i];
        comboBoxInterface_->addItem(interface.interfaceName, interface.interfaceIndex);

        adapters += interface.interfaceName;

        if (i < macAddrSpoofing.networkInterfaces.size() - 1) adapters += ", ";
    }

    qCDebug(LOG_BASIC) << "Updating Spoofing adapter dropdown with: " << adapters;
    qCDebug(LOG_BASIC) << "Spoofing selection: " << macAddrSpoofing.selectedNetworkInterface.interfaceName;

    comboBoxInterface_->setCurrentItem(macAddrSpoofing.selectedNetworkInterface.interfaceIndex);
}

void MacSpoofingItem::updatePositions()
{
    macAddressItem_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    autoRotateMacItem_->setPos(0, (COLLAPSED_HEIGHT + 43)*G_SCALE);
    comboBoxInterface_->setPos(0, (COLLAPSED_HEIGHT + 43 + 43)*G_SCALE);
}

} // namespace PreferencesWindow
