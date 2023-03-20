#include "securehotspotgroup.h"

#include <QPainter>
#include <QMessageBox>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

SecureHotspotGroup::SecureHotspotGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), supported_(HOTSPOT_SUPPORTED)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    checkBoxEnable_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Secure Hotspot"), "");
    checkBoxEnable_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SECURE_HOTSPOT"));
    connect(checkBoxEnable_, &CheckBoxItem::stateChanged, this, &SecureHotspotGroup::onCheckBoxStateChanged);
    addItem(checkBoxEnable_);

    editBoxSSID_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "SSID"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter SSID"));
    connect(editBoxSSID_, &EditBoxItem::textChanged, this, &SecureHotspotGroup::onSSIDChanged);
    addItem(editBoxSSID_);

    editBoxPassword_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Password"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter Password"));
    connect(editBoxPassword_, &EditBoxItem::textChanged, this, &SecureHotspotGroup::onPasswordChanged);
    addItem(editBoxPassword_);

    hideItems(indexOf(editBoxSSID_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    setSupported(supported_);
    updateDescription();
}

void SecureHotspotGroup::setSecureHotspotSettings(const types::ShareSecureHotspot &ss)
{
    if (settings_ == ss) {
        return;
    }

    settings_ = ss;
    checkBoxEnable_->setState(ss.isEnabled);
    editBoxSSID_->setText(ss.ssid);
    editBoxPassword_->setText(ss.password);
    updateMode();
}

void SecureHotspotGroup::setSupported(HOTSPOT_SUPPORT_TYPE supported)
{
    supported_ = supported;
    checkBoxEnable_->setEnabled(supported_ == HOTSPOT_SUPPORTED);
    if (supported_ != HOTSPOT_SUPPORTED)
    {
        checkBoxEnable_->setState(false);
        settings_.isEnabled = false;
        emit secureHotspotPreferencesChanged(settings_);
    }
    updateDescription();
    updateMode();
}

bool SecureHotspotGroup::hasItemWithFocus()
{
    return editBoxSSID_->lineEditHasFocus() || editBoxPassword_->lineEditHasFocus();
}

void SecureHotspotGroup::onCheckBoxStateChanged(bool isChecked)
{
    settings_.isEnabled = isChecked;
    updateMode();
    emit secureHotspotPreferencesChanged(settings_);
}

void SecureHotspotGroup::onSSIDChanged(const QString &text)
{
    settings_.ssid = text;
    emit secureHotspotPreferencesChanged(settings_);
}

void SecureHotspotGroup::onPasswordChanged(const QString &password)
{
    if (password.length() >= 8)
    {
        settings_.password = password;
        emit secureHotspotPreferencesChanged(settings_);
    }
    else
    {
        QString title = tr("Windscribe");
        QString desc = tr("Hotspot password must be at least 8 characters.");
        QMessageBox::information(g_mainWindow, title, desc);
        editBoxPassword_->setText(settings_.password);
    }
}

void SecureHotspotGroup::onLanguageChanged()
{
}

void SecureHotspotGroup::updateDescription()
{
    switch(supported_)
    {
        case HOTSPOT_NOT_SUPPORTED:
            setDescription(tr("Secure hotspot is not supported by your network adapter."));
            break;
        case HOTSPOT_NOT_SUPPORTED_BY_IKEV2:
            setDescription(tr("Secure hotspot is not supported for IKEv2/WireGuard protocol and automatic connection mode."));
            break;
        case HOTSPOT_SUPPORTED:
            setDescription(tr("Share your secure Windscribe connection wirelessly."));
            break;
    }
}

void SecureHotspotGroup::updateMode()
{
    if (checkBoxEnable_->isChecked())
    {
        showItems(indexOf(editBoxSSID_), indexOf(editBoxPassword_));
    }
    else
    {
        hideItems(indexOf(editBoxSSID_), indexOf(editBoxPassword_));
    }
}

} // namespace PreferencesWindow
