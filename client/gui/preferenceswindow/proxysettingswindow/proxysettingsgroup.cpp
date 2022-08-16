#include "proxysettingsgroup.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

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

    comboBoxProxyType_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Proxy"), "");
    const QList<QPair<QString, int>> types = PROXY_OPTION_toList();
    for (const auto p : types)
    {
        if (IsProxyOptionAllowed(p.second))
            comboBoxProxyType_->addItem(p.first, p.second);
    }
    comboBoxProxyType_->setCurrentItem(types.begin()->second);
    connect(comboBoxProxyType_, &ComboBoxItem::currentItemChanged, this, &ProxySettingsGroup::onProxyTypeChanged);
    comboBoxProxyType_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/PROXY"));
    addItem(comboBoxProxyType_);

    editBoxAddress_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Address"));
    connect(editBoxAddress_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onAddressChanged);
    addItem(editBoxAddress_);

    editBoxPort_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Port"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Port"));
    connect(editBoxPort_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onPortChanged);
    addItem(editBoxPort_);

    editBoxUsername_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Username"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Username"));
    connect(editBoxUsername_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onUsernameChanged);
    addItem(editBoxUsername_);

    editBoxPassword_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Password"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Password"));
    editBoxPassword_->setMasked(true);
    connect(editBoxPassword_, &EditBoxItem::textChanged, this, &ProxySettingsGroup::onPasswordChanged);
    addItem(editBoxPassword_);

    hideItems(indexOf(editBoxAddress_), indexOf(editBoxPassword_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);
}

void ProxySettingsGroup::onAddressChanged(const QString &text)
{
    if (settings_.address() != text)
    {
        settings_.setAddress(text);
        emit proxySettingsChanged(settings_);
    }
}

void ProxySettingsGroup::onPortChanged(const QString &text)
{
    if (QString::number(settings_.getPort()) != text)
    {
        settings_.setPort(text.toInt());
        emit proxySettingsChanged(settings_);
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
    if(settings_ != ps)
    {
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
    if (settings_.getPort() == 0)
    {
        editBoxPort_->setText("");
    }
    else
    {
        editBoxPort_->setText(QString::number(settings_.getPort()));
    }
    editBoxUsername_->setText(settings_.getUsername());
    editBoxPassword_->setText(settings_.getPassword());
}

void ProxySettingsGroup::onProxyTypeChanged(QVariant v)
{
    int newOption = v.toInt();
    if (!IsProxyOptionAllowed(newOption))
    {
        newOption = PROXY_OPTION_NONE;
    }

    if (newOption != settings_.option())
    {
        if (newOption == PROXY_OPTION_HTTP || newOption == PROXY_OPTION_SOCKS)
        {
            showItems(indexOf(editBoxAddress_), indexOf(editBoxPassword_));
        }
        else if (newOption == PROXY_OPTION_NONE || newOption == PROXY_OPTION_AUTODETECT)
        {
            hideItems(indexOf(editBoxAddress_), indexOf(editBoxPassword_));
        }

        settings_.setOption((PROXY_OPTION)newOption);
        emit proxySettingsChanged(settings_);
    }
}


} // namespace PreferencesWindow
