#include "proxygatewaygroup.h"

#include <QHostAddress>
#include <QPainter>
#include "generalmessagecontroller.h"
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
    comboBoxProxyType_->setCaptionFont(FontDescr(14, QFont::Normal));
    connect(comboBoxProxyType_, &ComboBoxItem::currentItemChanged, this, &ProxyGatewayGroup::onProxyTypeItemChanged);
    addItem(comboBoxProxyType_);

    editBoxPort_ = new EditBoxItem(this);
    editBoxPort_->setText(tr("Auto"));

    QRegularExpression ipRegex ("^\\d+$");
    QRegularExpressionValidator *integerValidator = new QRegularExpressionValidator(ipRegex, this);
    editBoxPort_->setValidator(integerValidator);

    connect(editBoxPort_, &EditBoxItem::textChanged, this, &ProxyGatewayGroup::onPortChanged);
    connect(editBoxPort_, &EditBoxItem::cancelled, this, &ProxyGatewayGroup::onPortCancelClicked);
    connect(editBoxPort_, &EditBoxItem::editClicked, this, &ProxyGatewayGroup::onPortEditClicked);
    addItem(editBoxPort_);

    proxyIpAddressItem_ = new ProxyIpAddressItem(this);
    addItem(proxyIpAddressItem_);

    checkBoxWhileConnected_ = new ToggleItem(this);
    connect(checkBoxWhileConnected_, &ToggleItem::stateChanged, this, &ProxyGatewayGroup::onWhileConnectedStateChanged);
    addItem(checkBoxWhileConnected_);

    hideItems(indexOf(comboBoxProxyType_), indexOf(checkBoxWhileConnected_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

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
        if (sp.port == 0) {
            editBoxPort_->setText(tr("Auto"));
        } else {
            editBoxPort_->setText(QString::number(sp.port));
        }
        checkBoxWhileConnected_->setState(sp.whileConnected);
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

void ProxyGatewayGroup::onPortCancelClicked()
{
    if (settings_.port == 0) {
        editBoxPort_->setText(tr("Auto"));
    }
}

void ProxyGatewayGroup::onPortChanged(QVariant v)
{
    if (v.toInt() == settings_.port) {
        if (v.toInt() == 0) {
            editBoxPort_->setText(tr("Auto"));
        }
        return;
    }

    if (v.toInt() > 0 && v.toInt() <= 65535) {
        settings_.port = v.toInt();
    } else {
        editBoxPort_->setText(tr("Auto"));
        settings_.port = 0;
    }
    emit proxyGatewayPreferencesChanged(settings_);
}

void ProxyGatewayGroup::onWhileConnectedStateChanged(bool isChecked)
{
    settings_.whileConnected = isChecked;
    emit proxyGatewayPreferencesChanged(settings_);
}

void ProxyGatewayGroup::setProxyGatewayAddress(const QString &address)
{
    if (settings_.isEnabled && address.endsWith(":0")) {
        // Port 0 indicates that the proxy is not running.  Reset to Auto.
        editBoxPort_->setText(tr("Auto"));
        settings_.port = 0;
        emit proxyGatewayPreferencesChanged(settings_);

        GeneralMessageController::instance().showMessage(
            "WARNING_YELLOW",
            tr("Unable to start proxy server"),
            tr("The proxy server couldn't be started on the requested port. Please try again with a different port."),
            GeneralMessageController::tr(GeneralMessageController::kOk),
            "",
            "",
            std::function<void(bool b)>(nullptr),
            std::function<void(bool b)>(nullptr),
            std::function<void(bool b)>(nullptr),
            GeneralMessage::kFromPreferences);
        return;
    }
    proxyIpAddressItem_->setIP(address);
}

void ProxyGatewayGroup::hideOpenPopups()
{
    comboBoxProxyType_->hideMenu();
}

void ProxyGatewayGroup::updateMode()
{
    if (checkBoxEnable_->isChecked()) {
        showItems(indexOf(comboBoxProxyType_), indexOf(checkBoxWhileConnected_));
    }
    else {
        hideItems(indexOf(comboBoxProxyType_), indexOf(checkBoxWhileConnected_));
    }
}

void ProxyGatewayGroup::onLanguageChanged()
{
    checkBoxEnable_->setCaption(tr("Proxy Gateway"));
    comboBoxProxyType_->setLabelCaption(tr("Proxy Type"));
    comboBoxProxyType_->setItems(PROXY_SHARING_TYPE_toList(), settings_.proxySharingMode);
    checkBoxWhileConnected_->setCaption(tr("Only when VPN is connected"));
    editBoxPort_->setCaption(tr("Port"));
}

void ProxyGatewayGroup::onPortEditClicked()
{
    if (settings_.port == 0) {
        editBoxPort_->setText(QString::number(0));
    }
}

void ProxyGatewayGroup::setDescription(const QString &desc, const QString &descUrl)
{
    checkBoxEnable_->setDescription(desc, descUrl);
}

} // namespace PreferencesWindow
