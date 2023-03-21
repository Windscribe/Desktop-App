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

    advParametersGroup_ = new PreferenceGroup(this);
    advParametersItem_ = new LinkItem(advParametersGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(advParametersItem_, &LinkItem::clicked, this, &AdvancedWindowItem::advParametersClick);
    advParametersGroup_->addItem(advParametersItem_);
    addItem(advParametersGroup_);

#ifdef Q_OS_WIN
    tapAdapterGroup_ = new PreferenceGroup(this);
    comboBoxTapAdapter_ = new ComboBoxItem(tapAdapterGroup_);
    comboBoxTapAdapter_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TAP_DRIVER"));
    updateTapAdaptersList();
    comboBoxTapAdapter_->setCurrentItem((int)preferences_->tapAdapter());
    connect(comboBoxTapAdapter_, &ComboBoxItem::currentItemChanged, this, &AdvancedWindowItem::onTapAdapterChanged);
    tapAdapterGroup_->addItem(comboBoxTapAdapter_);
    addItem(tapAdapterGroup_);

    ipv6Group_ = new PreferenceGroup(this);
    checkBoxIPv6_ = new CheckBoxItem(ipv6Group_);
    checkBoxIPv6_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/IPV6"));
    checkBoxIPv6_->setState(preferencesHelper->getIpv6StateInOS());
    connect(checkBoxIPv6_, &CheckBoxItem::stateChanged, this, &AdvancedWindowItem::onIPv6StateChanged);
    ipv6Group_->addItem(checkBoxIPv6_);
    addItem(ipv6Group_);
#endif

    apiResolutionGroup_ = new ApiResolutionGroup(this);
    apiResolutionGroup_->setApiResolution(preferences->apiResolution());
    connect(apiResolutionGroup_, &ApiResolutionGroup::apiResolutionChanged, this, &AdvancedWindowItem::onApiResolutionChanged);
    addItem(apiResolutionGroup_);

    ignoreSslErrorsGroup_ = new PreferenceGroup(this);
    cbIgnoreSslErrors_ = new CheckBoxItem(ignoreSslErrorsGroup_);
    cbIgnoreSslErrors_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/IGNORE_SSL_ERRORS"));
    cbIgnoreSslErrors_->setState(preferences->isIgnoreSslErrors());
    connect(cbIgnoreSslErrors_, &CheckBoxItem::stateChanged, this, &AdvancedWindowItem::onIgnoreSslErrorsStateChanged);
    ignoreSslErrorsGroup_->addItem(cbIgnoreSslErrors_);
    addItem(ignoreSslErrorsGroup_);

    keepAliveGroup_ = new PreferenceGroup(this);
    cbKeepAlive_ = new CheckBoxItem(keepAliveGroup_);
    cbKeepAlive_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/KEEPALIVE"));
    cbKeepAlive_->setState(preferences->keepAlive());
    connect(cbKeepAlive_, &CheckBoxItem::stateChanged, this, &AdvancedWindowItem::onKeepAliveStateChanged);
    keepAliveGroup_->addItem(cbKeepAlive_);
    addItem(keepAliveGroup_);

    internalDnsGroup_ = new PreferenceGroup(this);
    comboBoxAppInternalDns_ = new ComboBoxItem(internalDnsGroup_);
    comboBoxAppInternalDns_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/INTERNAL_DNS"));
    connect(comboBoxAppInternalDns_, &ComboBoxItem::currentItemChanged, this, &AdvancedWindowItem::onAppInternalDnsItemChanged);
    internalDnsGroup_->addItem(comboBoxAppInternalDns_);
    addItem(internalDnsGroup_);

#ifdef Q_OS_LINUX
    dnsManagerGroup_ = new PreferenceGroup(this);
    comboBoxDnsManager_ = new ComboBoxItem(dnsManagerGroup_);
    comboBoxDnsManager_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/DNS_MANAGER"));
    connect(comboBoxDnsManager_, &ComboBoxItem::currentItemChanged, this, &AdvancedWindowItem::onDnsManagerItemChanged);
    dnsManagerGroup_->addItem(comboBoxDnsManager_);
    addItem(dnsManagerGroup_);
#endif

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &AdvancedWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString AdvancedWindowItem::caption() const
{
    return tr("Advanced Options");
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
    advParametersGroup_->setDescription(tr("Make advanced tweaks to the way the app functions."));
    advParametersItem_->setTitle(tr("Advanced Parameters"));

#ifdef Q_OS_WIN
    tapAdapterGroup_->setDescription(tr("Pick between the TAP and Wintun network adapters for OpenVPN connections."));
    comboBoxTapAdapter_->setLabelCaption(tr("TAP Driver"));

    ipv6Group_->setDescription(tr("Enables / disables system-wide IPv6 connectivity."));
    checkBoxIPv6_->setCaption(tr("IPv6"));
#endif

    apiResolutionGroup_->setDescription(tr("Resolve server API address automatically, or use one provided by the Support team."));
    ignoreSslErrorsGroup_->setDescription(tr("Ignore SSL certificate validation errors."));
    cbIgnoreSslErrors_->setCaption(tr("Ignore SSL Errors"));
    keepAliveGroup_->setDescription(tr("Prevents IKEv2 connections from dying (by time-out) by periodically pinging the server."));
    cbKeepAlive_->setCaption(tr("Client-side Keepalive"));
    internalDnsGroup_->setDescription(tr("Windscribe uses this DNS server to resolve addresses outside the VPN.") + "\n" + tr("Warning: Using \"OS Default\" may sometimes cause DNS leaks during reconnects."));
    comboBoxAppInternalDns_->setLabelCaption(tr("App Internal DNS"));
    comboBoxAppInternalDns_->setItems(DNS_POLICY_TYPE_toList(), preferences_->dnsPolicy());

#ifdef Q_OS_LINUX
    dnsManagerGroup_->setDescription(tr("Select the DNS system service Windscribe enforces. Experienced users only."));
    comboBoxDnsManager_->setLabelCaption(tr("DNS Manager"));
    comboBoxDnsManager_->setItems(DNS_MANAGER_TYPE_toList(), preferences_->dnsManager());
#endif
}

void AdvancedWindowItem::hideOpenPopups()
{
    CommonGraphics::BasePage::hideOpenPopups();

#ifdef Q_OS_WIN
    comboBoxTapAdapter_->hideMenu();
#endif
}

} // namespace PreferencesWindow
