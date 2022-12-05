#include "proxygatewaygroup.h"

#include <QPainter>

#include "graphicresources/imageresourcessvg.h"

namespace PreferencesWindow {

ProxyGatewayGroup::ProxyGatewayGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    checkBoxEnable_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Proxy Gateway"), "");
    checkBoxEnable_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/PROXY_GATEWAY"));
    connect(checkBoxEnable_, &CheckBoxItem::stateChanged, this, &ProxyGatewayGroup::onCheckBoxStateChanged);
    addItem(checkBoxEnable_);

    comboBoxProxyType_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Proxy Type"), "");
    comboBoxProxyType_->setCaptionFont(FontDescr(12, false));
    const QList< QPair<QString, int> > allProxyTypes = PROXY_SHARING_TYPE_toList();
    for (const auto p : allProxyTypes) {
        comboBoxProxyType_->addItem(p.first, p.second);
    }
    comboBoxProxyType_->setCurrentItem(allProxyTypes.begin()->second);
    connect(comboBoxProxyType_, &ComboBoxItem::currentItemChanged, this, &ProxyGatewayGroup::onProxyTypeItemChanged);
    addItem(comboBoxProxyType_);
    hideItems(indexOf(comboBoxProxyType_));

    proxyIpAddressItem_ = new ProxyIpAddressItem(this);
    addItem(proxyIpAddressItem_);
    hideItems(indexOf(proxyIpAddressItem_));

    updateMode();
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

void ProxyGatewayGroup::onLanguageChanged()
{
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

} // namespace PreferencesWindow
