#include "advancedwindowitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMessageBox>
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencegroup.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/logger.h"
#include "utils/hardcodedsettings.h"

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

AdvancedWindowItem::AdvancedWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent), preferences_(preferences), preferencesHelper_(preferencesHelper),
    currentScreen_(ADVANCED_SCREEN_HOME)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

#ifdef Q_OS_WIN
    connect(preferences, &Preferences::tapAdapterChanged, this, &AdvancedWindowItem::onTapAdapterPreferencesChanged);
    connect(preferencesHelper, &PreferencesHelper::ipv6StateInOSChanged, this, &AdvancedWindowItem::onPreferencesIpv6InOSStateChanged);
#endif
    connect(preferences, &Preferences::apiResolutionChanged, this, &AdvancedWindowItem::onApiResolutionPreferencesChanged);
    connect(preferences, &Preferences::dnsPolicyChanged, this, &AdvancedWindowItem::onDnsPolicyPreferencesChanged);
#ifdef Q_OS_LINUX
    connect(preferences, &Preferences::dnsManagerChanged, this, &AdvancedWindowItem::onDnsManagerPreferencesChanged);
#endif
    connect(preferences, &Preferences::isIgnoreSslErrorsChanged, this, &AdvancedWindowItem::onIgnoreSslErrorsPreferencesChanged);
    connect(preferences, &Preferences::keepAliveChanged, this, &AdvancedWindowItem::onKeepAlivePreferencesChanged);

    advParametersGroup_ = new PreferenceGroup(this, "Make advanced tweaks to the way the app functions.", "");
    advParametersItem_ = new LinkItem(advParametersGroup_, LinkItem::LinkType::SUBPAGE_LINK, QT_TRANSLATE_NOOP("PreferencesWindow::LinkItem","Advanced Parameters"));
    connect(advParametersItem_, &LinkItem::clicked, this, &AdvancedWindowItem::advParametersClick);
    advParametersGroup_->addItem(advParametersItem_);
    addItem(advParametersGroup_);

#ifdef Q_OS_WIN
    tapAdapterGroup_ = new PreferenceGroup(this, "Pick between the TAP and Wintun network adapters for OpenVPN connections.", "");
    comboBoxTapAdapter_ = new ComboBoxItem(tapAdapterGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "TAP Driver"), QString());
    comboBoxTapAdapter_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TAP_DRIVER"));
    updateTapAdaptersList();
    comboBoxTapAdapter_->setCurrentItem((int)preferences_->tapAdapter());
    connect(comboBoxTapAdapter_, &ComboBoxItem::currentItemChanged, this, &AdvancedWindowItem::onTapAdapterChanged);
    tapAdapterGroup_->addItem(comboBoxTapAdapter_);
    addItem(tapAdapterGroup_);

    ipv6Group_ = new PreferenceGroup(this, "Enables / disables system-wide IPv6 connectivity.", "");
    checkBoxIPv6_ = new CheckBoxItem(ipv6Group_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "IPv6"), "");
    checkBoxIPv6_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/IPV6"));
    checkBoxIPv6_->setState(preferencesHelper->getIpv6StateInOS());
    connect(checkBoxIPv6_, &CheckBoxItem::stateChanged, this, &AdvancedWindowItem::onIPv6StateChanged);
    ipv6Group_->addItem(checkBoxIPv6_);
    addItem(ipv6Group_);
#endif

    apiResolutionGroup_ = new ApiResolutionGroup(this, "Resolve server API address automatically, or use an IP address provided by the Support team.");
    apiResolutionGroup_->setApiResolution(preferences->apiResolution());
    connect(apiResolutionGroup_, &ApiResolutionGroup::apiResolutionChanged, this, &AdvancedWindowItem::onApiResolutionChanged);
    addItem(apiResolutionGroup_);

    ignoreSslErrorsGroup_ = new PreferenceGroup(this, "Ignore SSL certificate validation errors.", "");
    cbIgnoreSslErrors_ = new CheckBoxItem(ignoreSslErrorsGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Ignore SSL Errors"), "");
    cbIgnoreSslErrors_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/IGNORE_SSL_ERRORS"));
    cbIgnoreSslErrors_->setState(preferences->isIgnoreSslErrors());
    connect(cbIgnoreSslErrors_, &CheckBoxItem::stateChanged, this, &AdvancedWindowItem::onIgnoreSslErrorsStateChanged);
    ignoreSslErrorsGroup_->addItem(cbIgnoreSslErrors_);
    addItem(ignoreSslErrorsGroup_);

    keepAliveGroup_ = new PreferenceGroup(this, "Prevents IKEv2 connections from dying (by time-out) by periodically pinging the server.", "");
    cbKeepAlive_ = new CheckBoxItem(keepAliveGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Client-side Keepalive"), "");
    cbKeepAlive_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/KEEPALIVE"));
    cbKeepAlive_->setState(preferences->keepAlive());
    connect(cbKeepAlive_, &CheckBoxItem::stateChanged, this, &AdvancedWindowItem::onKeepAliveStateChanged);
    keepAliveGroup_->addItem(cbKeepAlive_);
    addItem(keepAliveGroup_);

    internalDnsGroup_ = new PreferenceGroup(this, "Windscribe uses this DNS server to resolve addresses outside the VPN.");
    comboBoxAppInternalDns_ = new ComboBoxItem(internalDnsGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "App Internal DNS"), QString());
    comboBoxAppInternalDns_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/INTERNAL_DNS"));
    const QList< QPair<QString, int> > dnsTypes = DNS_POLICY_TYPE_toList();
    for (const auto &d : dnsTypes)
    {
        comboBoxAppInternalDns_->addItem(d.first, d.second);
    }
    comboBoxAppInternalDns_->setCurrentItem(preferences->dnsPolicy());
    connect(comboBoxAppInternalDns_, &ComboBoxItem::currentItemChanged, this, &AdvancedWindowItem::onAppInternalDnsItemChanged);
    internalDnsGroup_->addItem(comboBoxAppInternalDns_);
    addItem(internalDnsGroup_);

#ifdef Q_OS_LINUX
    dnsManagerGroup_ = new PreferenceGroup(this, "Select the DNS system service Windscribe enforces. Experienced users only.", "");
    comboBoxDnsManager_ = new ComboBoxItem(dnsManagerGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "DNS Manager"), QString());
    comboBoxDnsManager_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/DNS_MANAGER"));
    const QList< QPair<QString, int> > dnsManagerTypes = DNS_MANAGER_TYPE_toList();
    for (const auto &d : dnsManagerTypes)
    {
        comboBoxDnsManager_->addItem(d.first, d.second);
    }

    comboBoxDnsManager_->setCurrentItem(preferences->dnsManager());
    connect(comboBoxDnsManager_, &ComboBoxItem::currentItemChanged, this, &AdvancedWindowItem::onDnsManagerItemChanged);
    dnsManagerGroup_->addItem(comboBoxDnsManager_);
    addItem(dnsManagerGroup_);

#endif
}

QString AdvancedWindowItem::caption() const
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Advanced Options");
}

ADVANCED_SCREEN AdvancedWindowItem::getScreen()
{
    return currentScreen_;
}

void AdvancedWindowItem::setScreen(ADVANCED_SCREEN screen)
{
    currentScreen_ = screen;
}

void AdvancedWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
}

#ifdef Q_OS_WIN
void AdvancedWindowItem::onTapAdapterChanged(QVariant v)
{
    preferences_->setTapAdapter((TAP_ADAPTER_TYPE)v.toInt());
}

void AdvancedWindowItem::onIPv6StateChanged(bool isChecked)
{
    QMessageBox msgBox(g_mainWindow);
    msgBox.setText(tr("In order to disable IPv6, a computer restart is required. Do it now?"));
    QAbstractButton* pButtonYes = msgBox.addButton(tr("Yes"), QMessageBox::NoRole);
    QAbstractButton* pButtonLater = msgBox.addButton(tr("Restart later"), QMessageBox::NoRole);
    QAbstractButton* pButtonCancel = msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    msgBox.exec();

    if (msgBox.clickedButton() == pButtonYes)
    {
        emit setIpv6StateInOS(isChecked, true);
    }
    else if (msgBox.clickedButton() == pButtonLater)
    {
        emit setIpv6StateInOS(isChecked, false);
    }
    else if (msgBox.clickedButton() == pButtonCancel)
    {
        checkBoxIPv6_->setState(!isChecked);
    }
}
#endif

void AdvancedWindowItem::onApiResolutionChanged(const types::ApiResolutionSettings &ar)
{
    preferences_->setApiResolution(ar);
}

void AdvancedWindowItem::onIgnoreSslErrorsStateChanged(bool isChecked)
{
    preferences_->setIgnoreSslErrors(isChecked);
}

void AdvancedWindowItem::onKeepAliveStateChanged(bool isChecked)
{
    preferences_->setKeepAlive(isChecked);
}

void AdvancedWindowItem::onAppInternalDnsItemChanged(QVariant dns)
{
    preferences_->setDnsPolicy((DNS_POLICY_TYPE)dns.toInt());
}

#ifdef Q_OS_LINUX
void AdvancedWindowItem::onDnsManagerItemChanged(QVariant dns)
{
    preferences_->setDnsManager((DNS_MANAGER_TYPE)dns.toInt());
}
#endif

void AdvancedWindowItem::onIgnoreSslErrorsPreferencesChanged(bool b)
{
    cbIgnoreSslErrors_->setState(b);
}

void AdvancedWindowItem::onKeepAlivePreferencesChanged(bool b)
{
    cbKeepAlive_->setState(b);
}

void AdvancedWindowItem::onDnsPolicyPreferencesChanged(DNS_POLICY_TYPE d)
{
    comboBoxAppInternalDns_->setCurrentItem((int)d);
}

#ifdef Q_OS_LINUX
void AdvancedWindowItem::onDnsManagerPreferencesChanged(DNS_MANAGER_TYPE d)
{
    comboBoxDnsManager_->setCurrentItem((int)d);
}
#endif

void AdvancedWindowItem::onApiResolutionPreferencesChanged(const types::ApiResolutionSettings &ar)
{
    apiResolutionGroup_->setApiResolution(ar);
}

#ifdef Q_OS_WIN
void AdvancedWindowItem::onPreferencesIpv6InOSStateChanged(bool bEnabled)
{
    checkBoxIPv6_->setState(bEnabled);
}

void AdvancedWindowItem::onTapAdapterPreferencesChanged(TAP_ADAPTER_TYPE tapAdapter)
{
    comboBoxTapAdapter_->setCurrentItem((int)tapAdapter);
}

void AdvancedWindowItem::updateTapAdaptersList()
{
    comboBoxTapAdapter_->clear();
    comboBoxTapAdapter_->addItem(TAP_ADAPTER_TYPE_toString(TAP_ADAPTER), (int)TAP_ADAPTER);
    comboBoxTapAdapter_->addItem(TAP_ADAPTER_TYPE_toString(WINTUN_ADAPTER), (int)WINTUN_ADAPTER);
}
#endif

void AdvancedWindowItem::onLanguageChanged()
{
}

void AdvancedWindowItem::hideOpenPopups()
{
    CommonGraphics::BasePage::hideOpenPopups();

#ifdef Q_OS_WIN
    comboBoxTapAdapter_->hideMenu();
#endif
}

} // namespace PreferencesWindow
