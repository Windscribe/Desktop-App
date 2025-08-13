#include "advancedwindowitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencegroup.h"

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

AdvancedWindowItem::AdvancedWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent), preferences_(preferences), preferencesHelper_(preferencesHelper),
    currentScreen_(ADVANCED_SCREEN_HOME)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

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

    ignoreSslErrorsGroup_ = new PreferenceGroup(this);
    cbIgnoreSslErrors_ = new ToggleItem(ignoreSslErrorsGroup_);
    cbIgnoreSslErrors_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/IGNORE_SSL_ERRORS"));
    cbIgnoreSslErrors_->setState(preferences->isIgnoreSslErrors());
    connect(cbIgnoreSslErrors_, &ToggleItem::stateChanged, this, &AdvancedWindowItem::onIgnoreSslErrorsStateChanged);
    ignoreSslErrorsGroup_->addItem(cbIgnoreSslErrors_);
    addItem(ignoreSslErrorsGroup_);

    keepAliveGroup_ = new PreferenceGroup(this);
    cbKeepAlive_ = new ToggleItem(keepAliveGroup_);
    cbKeepAlive_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/KEEPALIVE"));
    cbKeepAlive_->setState(preferences->keepAlive());
    connect(cbKeepAlive_, &ToggleItem::stateChanged, this, &AdvancedWindowItem::onKeepAliveStateChanged);
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

    appPreferencesGroup_ = new PreferenceGroup(this);
    appPreferencesItem_ = new LinkItem(appPreferencesGroup_, LinkItem::LinkType::TEXT_ONLY);
    appPreferencesItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/APP_PREFERENCES"));
    appPreferencesGroup_->addItem(appPreferencesItem_);

    exportSettingsItem_ = new LinkItem(appPreferencesGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(exportSettingsItem_, &LinkItem::clicked, this, &AdvancedWindowItem::exportSettingsClick);
    appPreferencesGroup_->addItem(exportSettingsItem_);

    importSettingsItem_ = new LinkItem(appPreferencesGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(importSettingsItem_, &LinkItem::clicked, this, &AdvancedWindowItem::importSettingsClick);
    appPreferencesGroup_->addItem(importSettingsItem_);

    addItem(appPreferencesGroup_);

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

void AdvancedWindowItem::onLanguageChanged()
{
    advParametersItem_->setDescription(tr("Make advanced tweaks to the way the app functions."));
    advParametersItem_->setTitle(tr("Advanced Parameters"));
    cbIgnoreSslErrors_->setDescription(tr("Ignore SSL certificate validation errors."));
    cbIgnoreSslErrors_->setCaption(tr("Ignore SSL Errors"));
    cbKeepAlive_->setDescription(tr("Prevents connections from dying (by time-out) by periodically pinging the server."));
    cbKeepAlive_->setCaption(tr("Client-side Keepalive"));
    comboBoxAppInternalDns_->setDescription(tr("Windscribe uses this DNS server to resolve addresses outside the VPN.") + "\n" + tr("Warning: Using \"OS Default\" may sometimes cause DNS leaks during reconnects."));
    comboBoxAppInternalDns_->setLabelCaption(tr("App Internal DNS"));
    comboBoxAppInternalDns_->setItems(DNS_POLICY_TYPE_toList(), preferences_->dnsPolicy());

#ifdef Q_OS_LINUX
    comboBoxDnsManager_->setDescription(tr("Select the DNS system service Windscribe enforces. Experienced users only."));
    comboBoxDnsManager_->setLabelCaption(tr("DNS Manager"));
    comboBoxDnsManager_->setItems(DNS_MANAGER_TYPE_toList(), preferences_->dnsManager());
#endif

    appPreferencesItem_->setTitle(tr("App Preferences"));
    exportSettingsItem_->setTitle(tr("Export"));
    importSettingsItem_->setTitle(tr("Import"));
}

void AdvancedWindowItem::hideOpenPopups()
{
    CommonGraphics::BasePage::hideOpenPopups();
}

void AdvancedWindowItem::setPreferencesImportCompleted()
{
    importSettingsItem_->setLinkIcon(ImageResourcesSvg::instance().getIndependentPixmap("CHECKMARK"));

    // Revert to old icon after 5 seconds
    QTimer::singleShot(5000, [this](){
        importSettingsItem_->setLinkIcon(nullptr);
    });
}

} // namespace PreferencesWindow
