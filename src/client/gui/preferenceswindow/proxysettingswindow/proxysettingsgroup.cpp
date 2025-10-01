#include "proxysettingsgroup.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "generalmessagecontroller.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

static bool IsProxyOptionAllowed(int option_number) {
    // Temporarily hide Auto-detect and SOCKS proxy options in the GUI.
    return option_number != PROXY_OPTION_AUTODETECT &&
           option_number != PROXY_OPTION_SOCKS;
}

ProxySettingsGroup::ProxySettingsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxProxyType_ = new ComboBoxItem(this);
    connect(comboBoxProxyType_, &ComboBoxItem::currentItemChanged, this, &ProxySettingsGroup::onProxyTypeChanged);
    comboBoxProxyType_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/PROXY"));
    addItem(comboBoxProxyType_);

    editBoxAddress_ = new EditBoxItem(this);
    connect(editBoxAddress_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onAddressChanged);
    addItem(editBoxAddress_);

    editBoxPort_ = new EditBoxItem(this);
    connect(editBoxPort_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onPortChanged);
    addItem(editBoxPort_);

    editBoxUsername_ = new EditBoxItem(this);
    connect(editBoxUsername_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onUsernameChanged);
    addItem(editBoxUsername_);

    editBoxPassword_ = new EditBoxItem(this);
    editBoxPassword_->setMasked(true);
    connect(editBoxPassword_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onPasswordChanged);
    addItem(editBoxPassword_);

    hideItems(indexOf(editBoxAddress_), indexOf(editBoxPassword_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ProxySettingsGroup::onLanguageChanged);
    onLanguageChanged();
}

void ProxySettingsGroup::onAddressChanged(const QString &text)
{
    if (settings_.address() != text) {
        if (IpValidation::isIp(text.trimmed())) {
            settings_.setAddress(text);
            emit proxySettingsChanged(settings_);
        } else {
            // Not accepted, revert to previous value
            editBoxAddress_->setText(settings_.address());
            GeneralMessageController::instance().showMessage(
                "WARNING_WHITE",
                tr("Invalid proxy address"),
                tr("Proxy address is invalid. Please enter a valid IP address."),
                GeneralMessageController::tr(GeneralMessageController::kOk),
                "",
                "",
                std::function<void(bool b)>(nullptr),
                std::function<void(bool b)>(nullptr),
                std::function<void(bool b)>(nullptr),
                GeneralMessage::kFromPreferences);
        }
    }
}

void ProxySettingsGroup::onPortChanged(const QString &text)
{
    if (QString::number(settings_.getPort()) != text) {
        uint port = text.toUInt();
        if (port < 63536) {
            settings_.setPort(port);
            emit proxySettingsChanged(settings_);
        } else {
            GeneralMessageController::instance().showMessage(
                "WARNING_WHITE",
                tr("Invalid proxy port"),
                tr("Proxy port is invalid. Please enter a valid port in the range 0-65535."),
                GeneralMessageController::tr(GeneralMessageController::kOk),
                "",
                "",
                std::function<void(bool b)>(nullptr),
                std::function<void(bool b)>(nullptr),
                std::function<void(bool b)>(nullptr),
                GeneralMessage::kFromPreferences);
            editBoxPort_->setText(QString::number(settings_.getPort()));
        }
    }
}

void ProxySettingsGroup::onUsernameChanged(const QString &text)
{
    if (settings_.getUsername() != text)
    {
        settings_.setUsername(text);
        emit proxySettingsChanged(settings_);
    }
}

void ProxySettingsGroup::onPasswordChanged(const QString &text)
{
    if (settings_.getPassword() != text)
    {
        settings_.setPassword(text);
        emit proxySettingsChanged(settings_);
    }
}

void ProxySettingsGroup::setProxySettings(const types::ProxySettings &ps)
{
    if (settings_ == ps) {
        return;
    }

    bool optionChanged = (ps.option() != settings_.option());

    settings_ = ps;
    int curOption = settings_.option();
    if (optionChanged)
    {
        onProxyTypeChanged(curOption);
    }
    comboBoxProxyType_->setCurrentItem(curOption);

    editBoxAddress_->setText(settings_.address());
    editBoxPort_->setText(QString::number(settings_.getPort()));
    editBoxUsername_->setText(settings_.getUsername());
    editBoxPassword_->setText(settings_.getPassword());

    updateMode();
}

void ProxySettingsGroup::onProxyTypeChanged(QVariant v)
{
    int newOption = v.toInt();
    if (!IsProxyOptionAllowed(newOption))
    {
        newOption = PROXY_OPTION_NONE;
    }

    if (newOption != settings_.option()) {
        settings_.setOption((PROXY_OPTION)newOption);
        updateMode();
        emit proxySettingsChanged(settings_);
    }
}

void ProxySettingsGroup::updateMode()
{
    if (settings_.option() == PROXY_OPTION_HTTP || settings_.option() == PROXY_OPTION_SOCKS) {
        showItems(indexOf(editBoxAddress_), indexOf(editBoxPassword_));
    } else if (settings_.option() == PROXY_OPTION_NONE || settings_.option() == PROXY_OPTION_AUTODETECT) {
        hideItems(indexOf(editBoxAddress_), indexOf(editBoxPassword_));
    }
}

void ProxySettingsGroup::onLanguageChanged()
{
    comboBoxProxyType_->setLabelCaption(tr("Proxy"));
    QList<QPair<QString, QVariant>> list;
    for (const auto p : PROXY_OPTION_toList()) {
        if (IsProxyOptionAllowed(p.second.toInt())) {
            list << p;
        }
    }
    comboBoxProxyType_->setItems(list, settings_.option());
    editBoxAddress_->setCaption(tr("Address"));
    editBoxAddress_->setPrompt(tr("Address"));
    editBoxPort_->setCaption(tr("Port"));
    editBoxPort_->setPrompt(tr("Port"));
    editBoxUsername_->setCaption(tr("Username"));
    editBoxUsername_->setPrompt(tr("Username"));
    editBoxPassword_->setCaption(tr("Password"));
    editBoxPassword_->setPrompt(tr("Password"));
}


} // namespace PreferencesWindow
