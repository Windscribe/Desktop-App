#include "macspoofinggroup.h"

#include <QPainter>

#include "generalmessagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"

namespace PreferencesWindow {

MacSpoofingGroup::MacSpoofingGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    checkBoxEnable_ = new ToggleItem(this);
    checkBoxEnable_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/MAC_SPOOFING"));
    connect(checkBoxEnable_, &ToggleItem::stateChanged, this, &MacSpoofingGroup::onCheckBoxStateChanged);
    addItem(checkBoxEnable_);

    macAddressItem_ = new MacAddressItem(this);
    connect(macAddressItem_, &MacAddressItem::cycleMacAddressClick, this, &MacSpoofingGroup::onCycleMacAddressClick);
    addItem(macAddressItem_);

    comboBoxInterface_ = new ComboBoxItem(this);
    comboBoxInterface_ ->setCaptionFont(FontDescr(14, QFont::Normal));
    connect(comboBoxInterface_, &ComboBoxItem::currentItemChanged, this, &MacSpoofingGroup::onInterfaceItemChanged);
    addItem(comboBoxInterface_);

    autoRotateMacItem_ = new ToggleItem(this);
    connect(autoRotateMacItem_, &ToggleItem::stateChanged, this, &MacSpoofingGroup::onAutoRotateMacStateChanged);
    addItem(autoRotateMacItem_);

    hideItems(indexOf(macAddressItem_), indexOf(autoRotateMacItem_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &MacSpoofingGroup::onLanguageChanged);
    onLanguageChanged();
}

void MacSpoofingGroup::setMacSpoofingSettings(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing != settings_) {
        settings_ = macAddrSpoofing;

        checkBoxEnable_->setState(macAddrSpoofing.isEnabled);
        macAddressItem_->setMacAddress(macAddrSpoofing.macAddress);
        autoRotateMacItem_->setState(macAddrSpoofing.isAutoRotate);
        updateMode();
    }
}

void MacSpoofingGroup::setCurrentNetwork(const types::NetworkInterface &network)
{
    curNetwork_ = network;
}

void MacSpoofingGroup::onCheckBoxStateChanged(bool isChecked)
{
    qCDebug(LOG_USER) << "MAC Spoofing enabled: " << isChecked;
    settings_.isEnabled = isChecked;
    updateMode();
    emit macAddrSpoofingChanged(settings_);
}

void MacSpoofingGroup::onAutoRotateMacStateChanged(bool isChecked)
{
    qCDebug(LOG_USER) << "Auto-Rotate enabled: " << isChecked;
    settings_.isAutoRotate = isChecked;
    emit macAddrSpoofingChanged(settings_);
}

void MacSpoofingGroup::onInterfaceItemChanged(const QVariant &value)
{
    int interfaceIndex = value.toInt();

    for (const auto &interface : settings_.networkInterfaces) {
        if (interface.interfaceIndex == interfaceIndex) {
            settings_.selectedNetworkInterface = interface;
            break;
        }
    }

    // Automatically generate a new MAC address for the user, if we don't already have one,
    // so they don't have to manually click the cycle button.
    if (curNetwork_.interfaceIndex == settings_.selectedNetworkInterface.interfaceIndex &&
        curNetwork_.interfaceIndex != -1 && settings_.macAddress.isEmpty())
    {
        settings_.macAddress = NetworkUtils::generateRandomMacAddress();
        macAddressItem_->setMacAddress(settings_.macAddress);
        qCInfo(LOG_BASIC) << "Automatically generated a MAC address: " << settings_.macAddress;
    }

    qCInfo(LOG_USER) << "Changed MAC Spoof Interface Selection: " << settings_.selectedNetworkInterface.interfaceName;
    emit macAddrSpoofingChanged(settings_);
}

void MacSpoofingGroup::onCycleMacAddressClick()
{
    if (curNetwork_.interfaceIndex == settings_.selectedNetworkInterface.interfaceIndex) {
        if (curNetwork_.interfaceIndex != -1) {
            emit cycleMacAddressClick();
        } else {
            qCInfo(LOG_BASIC) << "Cannot spoof on 'No Interface'";
            GeneralMessageController::instance().showMessage(
                "ERROR_ICON",
                tr("Cannot spoof on 'No Interface'"),
                tr("You can only spoof an existing adapter."),
                GeneralMessageController::tr(GeneralMessageController::kOk),
                "",
                "",
                std::function<void(bool)>(nullptr),
                std::function<void(bool)>(nullptr),
                std::function<void(bool)>(nullptr),
                GeneralMessage::kFromPreferences);
        }
    } else {
        qCInfo(LOG_BASIC) << "Cannot spoof the current interface -- must match selected interface";
        GeneralMessageController::instance().showMessage(
            "ERROR_ICON",
            tr("Cannot spoof the current interface"),
            tr("The current primary interface must match the selected interface to spoof."),
            GeneralMessageController::tr(GeneralMessageController::kOk),
            "",
            "",
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            GeneralMessage::kFromPreferences);
    }
}

void MacSpoofingGroup::updateMode()
{
    if (settings_.isEnabled) {
        comboBoxInterface_->clear();
        bool spoofedAdapterExists = false;
        for (const auto &interface : settings_.networkInterfaces) {
            if (interface.interfaceName == "No Interface") {
                comboBoxInterface_->addItem(tr("No Interface"), interface.interfaceIndex);
            } else {
                comboBoxInterface_->addItem(interface.friendlyName, interface.interfaceIndex);
            }

            if (interface.interfaceIndex == settings_.selectedNetworkInterface.interfaceIndex) {
                spoofedAdapterExists = true;
            }
        }

        qCDebug(LOG_BASIC) << "Updating Spoofing adapter dropdown with: " << NetworkUtils::networkInterfacesToString(settings_.networkInterfaces, false);
        qCDebug(LOG_BASIC) << "Spoofing selection: " << settings_.selectedNetworkInterface.interfaceName;

        if (!spoofedAdapterExists) {
            // The adapter may have been unplugged/disabled by the user.  We still want to remember it as the spoofed
            // adapter, and thus display it as selected in the combobox, until the user elects to pick a different one.
            comboBoxInterface_->addItem(settings_.selectedNetworkInterface.friendlyName, settings_.selectedNetworkInterface.interfaceIndex);
        }

        comboBoxInterface_->setCurrentItem(settings_.selectedNetworkInterface.interfaceIndex);
        showItems(indexOf(macAddressItem_), size() - 1);
    }
    else {
        hideItems(indexOf(macAddressItem_), size() - 1);
    }
}

void MacSpoofingGroup::onLanguageChanged()
{
    checkBoxEnable_->setCaption(tr("MAC Spoofing"));
    macAddressItem_->setCaption(tr("MAC Address"));
    comboBoxInterface_->setLabelCaption(tr("Interface"));
    autoRotateMacItem_->setCaption(tr("Auto-Rotate MAC"));
    updateMode();
}

void MacSpoofingGroup::setEnabled(bool enabled)
{
    checkBoxEnable_->setEnabled(enabled);
    if (!enabled) {
        checkBoxEnable_->setState(false);
    }
}

void MacSpoofingGroup::setDescription(const QString &desc, const QString &descUrl)
{
    checkBoxEnable_->setDescription(desc, descUrl);
}

} // namespace PreferencesWindow
