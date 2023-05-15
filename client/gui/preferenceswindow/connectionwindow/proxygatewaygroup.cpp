#include "proxygatewaygroup.h"

#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

ProxyGatewayGroup::ProxyGatewayGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    checkBoxEnable_ = new ToggleItem(this);
    checkBoxEnable_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/PROXY_GATEWAY"));
    connect(checkBoxEnable_, &ToggleItem::stateChanged, this, &ProxyGatewayGroup::onCheckBoxStateChanged);
    addItem(checkBoxEnable_);

    comboBoxProxyType_ = new ComboBoxItem(this);
    comboBoxProxyType_->setCaptionFont(FontDescr(12, false));
    connect(comboBoxProxyType_, &ComboBoxItem::currentItemChanged, this, &ProxyGatewayGroup::onProxyTypeItemChanged);
    addItem(comboBoxProxyType_);

    proxyIpAddressItem_ = new ProxyIpAddressItem(this);
    addItem(proxyIpAddressItem_);

    hideItems(indexOf(comboBoxProxyType_), indexOf(proxyIpAddressItem_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    updateMode();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ProxyGatewayGroup::onLanguageChanged);
    onLanguageChanged();
}

void ProxyGatewayGroup::setProxyGatewaySettings(const types::ShareProxyGateway &sp)
{
    if (settings_ != sp) {
        settings_ = sp;
        checkBoxEnable_->setState(sp.isEnabled);
        comboBoxProxyType_->setCurrentItem(sp.proxySharingMode);
        updateMode();
    }
}

void ProxyGatewayGroup::onCheckBoxStateChanged(bool isChecked)
{
    settings_.isEnabled = isChecked;
    updateMode();
    emit proxyGatewayPreferencesChanged(settings_);
}

void ProxyGatewayGroup::onProxyTypeItemChanged(QVariant v)
{
    settings_.proxySharingMode = (PROXY_SHARING_TYPE)v.toInt();
    emit proxyGatewayPreferencesChanged(settings_);
}

void ProxyGatewayGroup::setProxyGatewayAddress(const QString &address)
{
    proxyIpAddressItem_->setIP(address);
}

void ProxyGatewayGroup::hideOpenPopups()
{
    comboBoxProxyType_->hideMenu();
}

void ProxyGatewayGroup::updateMode()
{
    if (checkBoxEnable_->isChecked()) {
        showItems(indexOf(comboBoxProxyType_), indexOf(proxyIpAddressItem_));
    }
    else {
        hideItems(indexOf(comboBoxProxyType_), indexOf(proxyIpAddressItem_));
    }
}

void ProxyGatewayGroup::onLanguageChanged()
{
    checkBoxEnable_->setCaption(tr("Proxy Gateway"));
    comboBoxProxyType_->setLabelCaption(tr("Proxy Type"));
    comboBoxProxyType_->setItems(PROXY_SHARING_TYPE_toList(), settings_.proxySharingMode);
}

} // namespace PreferencesWindow
