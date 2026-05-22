#include "proxygatewaygroup.h"

#include <QHostAddress>
#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "utils/utils.h"

namespace PreferencesWindow {

constexpr int kProxyCredentialLength = 12;
const QString kProxyUsername = QStringLiteral("windscribe");

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

    proxyIpAddressItem_ = new CopyableTextItem(this);
    addItem(proxyIpAddressItem_);

    checkBoxWhileConnected_ = new ToggleItem(this);
    connect(checkBoxWhileConnected_, &ToggleItem::stateChanged, this, &ProxyGatewayGroup::onWhileConnectedStateChanged);
    addItem(checkBoxWhileConnected_);

    checkBoxRequireAuth_ = new ToggleItem(this);
    connect(checkBoxRequireAuth_, &ToggleItem::stateChanged, this, &ProxyGatewayGroup::onRequireAuthStateChanged);
    addItem(checkBoxRequireAuth_);

    usernameItem_ = new CopyableTextItem(this);
    addItem(usernameItem_);

    passwordItem_ = new CopyableTextItem(this, "preferences/REFRESH_ICON");
    connect(passwordItem_, &CopyableTextItem::extraButtonPressed, this, &ProxyGatewayGroup::onRegeneratePasswordClicked);
    addItem(passwordItem_);

    hideItems(indexOf(comboBoxProxyType_), indexOf(passwordItem_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    updateMode();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ProxyGatewayGroup::onLanguageChanged);
    onLanguageChanged();
}

void ProxyGatewayGroup::setProxyGatewaySettings(const types::ShareProxyGateway &sp)
{
    settings_ = sp;
    checkBoxEnable_->setState(sp.isEnabled);
    comboBoxProxyType_->setCurrentItem(sp.proxySharingMode);
    if (sp.port == 0) {
        editBoxPort_->setText(tr("Auto"));
    } else {
        editBoxPort_->setText(QString::number(sp.port));
    }
    checkBoxWhileConnected_->setState(sp.whileConnected);
    checkBoxRequireAuth_->setState(sp.requireAuth);
    usernameItem_->setText(sp.username);
    passwordItem_->setText(sp.password);
    updateMode();
}

void ProxyGatewayGroup::onCheckBoxStateChanged(bool isChecked)
{
    settings_.isEnabled = isChecked;
    if (isChecked && settings_.requireAuth) {
        // Generate creds the first time the proxy is enabled with auth already on (fresh install: requireAuth defaults
        // to true). For legacy upgraders requireAuth is already false here, so this is a no-op.
        ensureCredentials();
    }
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
    // Engine returns "ip:port"; if it has no usable bind IP yet (no network, or whileConnected without VPN
    // and no primary interface) we get just ":port", which renders as a confusing leading-colon stub. Show a
    // friendly placeholder instead.
    if (address.startsWith(":")) {
        proxyIpAddressItem_->setText(tr("Unavailable"));
    } else {
        proxyIpAddressItem_->setText(address);
    }
}

void ProxyGatewayGroup::hideOpenPopups()
{
    comboBoxProxyType_->hideMenu();
}

void ProxyGatewayGroup::updateMode()
{
    if (!checkBoxEnable_->isChecked()) {
        hideItems(indexOf(comboBoxProxyType_), indexOf(passwordItem_));
        return;
    }
    showItems(indexOf(comboBoxProxyType_), indexOf(checkBoxRequireAuth_));
    if (settings_.requireAuth) {
        showItems(indexOf(usernameItem_), indexOf(passwordItem_));
    } else {
        hideItems(indexOf(usernameItem_), indexOf(passwordItem_));
    }
}

void ProxyGatewayGroup::onLanguageChanged()
{
    checkBoxEnable_->setCaption(tr("Proxy Gateway"));
    comboBoxProxyType_->setLabelCaption(tr("Proxy Type"));
    comboBoxProxyType_->setItems(PROXY_SHARING_TYPE_toList(), settings_.proxySharingMode);
    checkBoxWhileConnected_->setCaption(tr("Only when VPN is connected"));
    editBoxPort_->setCaption(tr("Port"));
    checkBoxRequireAuth_->setCaption(tr("Require Authentication"));
    checkBoxRequireAuth_->setDescription(
        tr("Without authentication, anyone on your network (e.g. coffee shop Wi-Fi) can use this proxy. "
           "Leave this on unless you know what you're doing."));
    proxyIpAddressItem_->setLabel(tr("IP"));
    usernameItem_->setLabel(tr("Username"));
    passwordItem_->setLabel(tr("Password"));
}

void ProxyGatewayGroup::onRequireAuthStateChanged(bool isChecked)
{
    settings_.requireAuth = isChecked;
    if (isChecked) {
        ensureCredentials();
    }
    updateMode();
    emit proxyGatewayPreferencesChanged(settings_);
}

void ProxyGatewayGroup::ensureCredentials()
{
    if (settings_.username.isEmpty() || settings_.password.isEmpty()) {
        settings_.username = kProxyUsername;
        settings_.password = Utils::generateSecureCredential(kProxyCredentialLength);
        usernameItem_->setText(settings_.username);
        passwordItem_->setText(settings_.password);
    }
}

void ProxyGatewayGroup::onRegeneratePasswordClicked()
{
    settings_.password = Utils::generateSecureCredential(kProxyCredentialLength);
    passwordItem_->setText(settings_.password);
    emit proxyGatewayPreferencesChanged(settings_);
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
