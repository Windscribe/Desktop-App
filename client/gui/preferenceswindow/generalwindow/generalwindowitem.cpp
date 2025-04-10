#include "generalwindowitem.h"

#include <QDesktopServices>
#include <QPainter>
#include <QSystemTrayIcon>
#include "utils/hardcodedsettings.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencegroup.h"

#if defined Q_OS_LINUX
    #include "utils/linuxutils.h"
#endif

namespace PreferencesWindow {

GeneralWindowItem::GeneralWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : CommonGraphics::BasePage(parent),
    preferences_(preferences), preferencesHelper_(preferencesHelper)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    connect(preferences, &Preferences::isLaunchOnStartupChanged, this, &GeneralWindowItem::onIsLaunchOnStartupPreferencesChanged);
    connect(preferences, &Preferences::isShowNotificationsChanged, this, &GeneralWindowItem::onIsShowNotificationsPreferencesChanged);
    connect(preferences, &Preferences::isDockedToTrayChanged, this, &GeneralWindowItem::onIsDockedToTrayPreferencesChanged);
    connect(preferences, &Preferences::languageChanged, this, &GeneralWindowItem::onLanguagePreferencesChanged);
    connect(preferences, &Preferences::locationOrderChanged, this, &GeneralWindowItem::onLocationOrderPreferencesChanged);
    connect(preferences, &Preferences::latencyDisplayChanged, this, &GeneralWindowItem::onLatencyDisplayPreferencesChanged);
    connect(preferences, &Preferences::updateChannelChanged, this, &GeneralWindowItem::onUpdateChannelPreferencesChanged);
    connect(preferences, &Preferences::backgroundSettingsChanged, this, &GeneralWindowItem::onPreferencesBackgroundSettingsChanged);
    connect(preferences, &Preferences::isStartMinimizedChanged, this, &GeneralWindowItem::onStartMinimizedPreferencesChanged);
    connect(preferences, &Preferences::showLocationLoadChanged, this, &GeneralWindowItem::onShowLocationLoadPreferencesChanged);
    connect(preferences, &Preferences::appSkinChanged, this, &GeneralWindowItem::onAppSkinPreferencesChanged);
    connect(preferences, &Preferences::minimizeAndCloseToTrayChanged, this, &GeneralWindowItem::onMinimizeAndCloseToTrayPreferencesChanged);
#if defined Q_OS_MACOS
    connect(preferences, &Preferences::hideFromDockChanged, this, &GeneralWindowItem::onHideFromDockPreferecesChanged);
    connect(preferences, &Preferences::multiDesktopBehaviorChanged, this, &GeneralWindowItem::onMultiDesktopBehaviorPreferencesChanged);
#elif defined Q_OS_LINUX
    connect(preferences, &Preferences::trayIconColorChanged, this, &GeneralWindowItem::onPreferencesTrayIconColorChanged);
#endif

    launchOnStartGroup_ = new PreferenceGroup(this);
    checkBoxLaunchOnStart_ = new ToggleItem(launchOnStartGroup_);
    checkBoxLaunchOnStart_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LAUNCH_AT_STARTUP"));
    checkBoxLaunchOnStart_->setState(preferences->isLaunchOnStartup());
    connect(checkBoxLaunchOnStart_, &ToggleItem::stateChanged, this, &GeneralWindowItem::onIsLaunchOnStartupClicked);
    launchOnStartGroup_->addItem(checkBoxLaunchOnStart_);
    addItem(launchOnStartGroup_);

    startMinimizedGroup_ = new PreferenceGroup(this);
    checkBoxStartMinimized_ = new ToggleItem(startMinimizedGroup_);
    checkBoxStartMinimized_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/START_MINIMIZED"));
    checkBoxStartMinimized_->setState(preferences->isStartMinimized());
    connect(checkBoxStartMinimized_, &ToggleItem::stateChanged, this, &GeneralWindowItem::onStartMinimizedClicked);
    startMinimizedGroup_->addItem(checkBoxStartMinimized_);
    addItem(startMinimizedGroup_);

    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        closeToTrayGroup_ = new PreferenceGroup(this);
        checkBoxMinimizeAndCloseToTray_ = new ToggleItem(closeToTrayGroup_);
        checkBoxMinimizeAndCloseToTray_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/MINIMIZE_AND_CLOSE_TO_TRAY"));
        checkBoxMinimizeAndCloseToTray_->setState(preferences->isMinimizeAndCloseToTray());
        connect(checkBoxMinimizeAndCloseToTray_, &ToggleItem::stateChanged, this, &GeneralWindowItem::onMinimizeAndCloseToTrayClicked);
        closeToTrayGroup_->addItem(checkBoxMinimizeAndCloseToTray_);
        addItem(closeToTrayGroup_);
    } else {
        closeToTrayGroup_ = nullptr;
        checkBoxMinimizeAndCloseToTray_ = nullptr;
    }

#if defined Q_OS_MACOS
    hideFromDockGroup_ = new PreferenceGroup(this);
    checkBoxHideFromDock_ = new ToggleItem(hideFromDockGroup_);
    checkBoxHideFromDock_->setState(preferences->isHideFromDock());
    checkBoxHideFromDock_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/HIDE_FROM_DOCK"));
    connect(checkBoxHideFromDock_, &ToggleItem::stateChanged, this, &GeneralWindowItem::onHideFromDockClicked);
    hideFromDockGroup_->addItem(checkBoxHideFromDock_);
    addItem(hideFromDockGroup_);
#endif

#ifndef Q_OS_LINUX
    dockedGroup_ = new PreferenceGroup(this);
    checkBoxDockedToTray_ = new ToggleItem(dockedGroup_);
    checkBoxDockedToTray_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/DOCKED"));
    checkBoxDockedToTray_->setState(preferences->isDockedToTray());
    connect(checkBoxDockedToTray_, &ToggleItem::stateChanged, this, &GeneralWindowItem::onDockedToTrayChanged);
    dockedGroup_->addItem(checkBoxDockedToTray_);
    addItem(dockedGroup_);
#endif

    showNotificationsGroup_ = new PreferenceGroup(this);
    checkBoxShowNotifications_ = new ToggleItem(showNotificationsGroup_);
    checkBoxShowNotifications_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SHOW_NOTIFICATIONS"));
    checkBoxShowNotifications_->setState(preferences->isShowNotifications());
    connect(checkBoxShowNotifications_, &ToggleItem::stateChanged, this, &GeneralWindowItem::onIsShowNotificationsClicked);
    showNotificationsGroup_->addItem(checkBoxShowNotifications_);
    addItem(showNotificationsGroup_);

    showLocationLoadGroup_ = new PreferenceGroup(this);
    checkBoxShowLocationLoad_ = new ToggleItem(showLocationLoadGroup_);
    checkBoxShowLocationLoad_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LOCATION_LOAD"));
    checkBoxShowLocationLoad_->setState(preferences->isShowLocationLoad());
    connect(checkBoxShowLocationLoad_, &ToggleItem::stateChanged, this, &GeneralWindowItem::onShowLocationLoadClicked);
    showLocationLoadGroup_->addItem(checkBoxShowLocationLoad_);
    addItem(showLocationLoadGroup_);

    locationOrderGroup_ = new PreferenceGroup(this);
    comboBoxLocationOrder_ = new ComboBoxItem(locationOrderGroup_);
    comboBoxLocationOrder_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LOCATION_ORDER"));
    connect(comboBoxLocationOrder_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onLocationItemChanged);
    locationOrderGroup_->addItem(comboBoxLocationOrder_);
    addItem(locationOrderGroup_);

    latencyDisplayGroup_ = new PreferenceGroup(this);
    comboBoxLatencyDisplay_ = new ComboBoxItem(latencyDisplayGroup_);
    comboBoxLatencyDisplay_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LATENCY_DISPLAY"));
    connect(comboBoxLatencyDisplay_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onLatencyItemChanged);
    latencyDisplayGroup_->addItem(comboBoxLatencyDisplay_);
    addItem(latencyDisplayGroup_);

#if defined(Q_OS_MACOS)
    multiDesktopBehaviorGroup_ = new PreferenceGroup(this,
                                                     QString(),
                                                     "https://github.com/Windscribe/Desktop-App/wiki/macOS-Multi%E2%80%90desktop-preference");
    multiDesktopBehaviorItem_ = new ComboBoxItem(multiDesktopBehaviorGroup_);
    multiDesktopBehaviorItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/MULTI_DESKTOP"));
    connect(multiDesktopBehaviorItem_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onMultiDesktopBehaviorChanged);
    multiDesktopBehaviorGroup_->addItem(multiDesktopBehaviorItem_);
    addItem(multiDesktopBehaviorGroup_);
#endif

    languageGroup_ = new PreferenceGroup(this);
    comboBoxLanguage_ = new ComboBoxItem(languageGroup_);
    comboBoxLanguage_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LANGUAGE"));
    connect(comboBoxLanguage_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onLanguageItemChanged);
    languageGroup_->addItem(comboBoxLanguage_);
    addItem(languageGroup_);

    appSkinGroup_ = new PreferenceGroup(this);
    appSkinItem_ = new ComboBoxItem(appSkinGroup_);
    appSkinItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/APP_SKIN"));
    connect(appSkinItem_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onAppSkinChanged);
    appSkinGroup_->addItem(appSkinItem_);
    addItem(appSkinGroup_);

#if defined(Q_OS_LINUX)
    trayIconColorGroup_ = new PreferenceGroup(this);
    trayIconColorItem_ = new ComboBoxItem(trayIconColorGroup_);
    // Change pixmap to the new one.
    trayIconColorItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TRAY_ICON_COLOR"));
    connect(trayIconColorItem_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onTrayIconColorChanged);
    trayIconColorGroup_->addItem(trayIconColorItem_);
    addItem(trayIconColorGroup_);
#endif
    backgroundSettingsGroup_ = new BackgroundSettingsGroup(this);
    backgroundSettingsGroup_->setBackgroundSettings(preferences->backgroundSettings());
    connect(backgroundSettingsGroup_, &BackgroundSettingsGroup::backgroundSettingsChanged, this, &GeneralWindowItem::onBackgroundSettingsChanged);
    addItem(backgroundSettingsGroup_);

    renameLocationsGroup_ = new PreferenceGroup(this);
    renameLocationsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::TEXT_ONLY);
    renameLocationsItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/RENAME_LOCATIONS"));
    renameLocationsGroup_->addItem(renameLocationsItem_);

    exportSettingsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(exportSettingsItem_, &LinkItem::clicked, this, &GeneralWindowItem::exportLocationNamesClick);
    renameLocationsGroup_->addItem(exportSettingsItem_);

    importSettingsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(importSettingsItem_, &LinkItem::clicked, this, &GeneralWindowItem::importLocationNamesClick);
    renameLocationsGroup_->addItem(importSettingsItem_);

    resetLocationsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(resetLocationsItem_, &LinkItem::clicked, this, &GeneralWindowItem::onResetLocationNamesClicked);
    renameLocationsGroup_->addItem(resetLocationsItem_);

    addItem(renameLocationsGroup_);

    updateChannelGroup_ = new PreferenceGroup(this,
                                              QString(),
                                              QString("https://%1/features/update-channels").arg(HardcodedSettings::instance().windscribeServerUrl()));
    comboBoxUpdateChannel_ = new ComboBoxItem(updateChannelGroup_);
    comboBoxUpdateChannel_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/UPDATE_CHANNEL"));
    connect(comboBoxUpdateChannel_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onUpdateChannelItemChanged);
    updateChannelGroup_->addItem(comboBoxUpdateChannel_);
    addItem(updateChannelGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &GeneralWindowItem::onLanguageChanged);

    versionGroup_ = new PreferenceGroup(this);
    versionGroup_->setDrawBackground(false);
    versionInfoItem_ = new VersionInfoItem(versionGroup_, QString(), preferencesHelper->buildVersion());
    connect(versionInfoItem_, &ClickableGraphicsObject::clicked, this, &GeneralWindowItem::onVersionInfoClicked);
    versionGroup_->addItem(versionInfoItem_);
    addItem(versionGroup_);

    // Populate combo boxes and other text
    onLanguageChanged();
}

QString GeneralWindowItem::caption() const
{
    return tr("General");
}

void GeneralWindowItem::onIsLaunchOnStartupClicked(bool isChecked)
{
    preferences_->setLaunchOnStartup(isChecked);
}

void GeneralWindowItem::onIsLaunchOnStartupPreferencesChanged(bool b)
{
    checkBoxLaunchOnStart_->setState(b);
}

void GeneralWindowItem::onStartMinimizedPreferencesChanged(bool b)
{
    checkBoxStartMinimized_->setState(b);
}

void GeneralWindowItem::onStartMinimizedClicked(bool b)
{
    if (preferences_)
        preferences_->setStartMinimized(b);
}

void GeneralWindowItem::onMinimizeAndCloseToTrayPreferencesChanged(bool b)
{
    checkBoxMinimizeAndCloseToTray_->setState(b);
}

void GeneralWindowItem::onMinimizeAndCloseToTrayClicked(bool b)
{
    preferences_->setMinimizeAndCloseToTray(b);
}

#if defined Q_OS_MACOS
void GeneralWindowItem::onHideFromDockPreferecesChanged(bool b)
{
    checkBoxHideFromDock_->setState(b);
}

void GeneralWindowItem::onHideFromDockClicked(bool b)
{
    preferences_->setHideFromDock(b);
}
#endif

void GeneralWindowItem::onIsShowNotificationsPreferencesChanged(bool b)
{
    checkBoxShowNotifications_->setState(b);
}

void GeneralWindowItem::onIsShowNotificationsClicked(bool b)
{
    preferences_->setShowNotifications(b);
}


void GeneralWindowItem::onIsDockedToTrayPreferencesChanged(bool b)
{
#ifndef Q_OS_LINUX
    checkBoxDockedToTray_->setState(b);
#endif
}

void GeneralWindowItem::onDockedToTrayChanged(bool b)
{
    preferences_->setDockedToTray(b);
}

void GeneralWindowItem::onLanguagePreferencesChanged(const QString &lang)
{
    comboBoxLanguage_->setCurrentItem(lang);

    LanguageController::instance().setLanguage(lang);
}

void GeneralWindowItem::onLanguageItemChanged(QVariant lang)
{
    preferences_->setLanguage(lang.toString());
}

void GeneralWindowItem::onLocationOrderPreferencesChanged(ORDER_LOCATION_TYPE o)
{
    comboBoxLocationOrder_->setCurrentItem((int)o);
}

void GeneralWindowItem::onLocationItemChanged(QVariant o)
{
    preferences_->setLocationOrder((ORDER_LOCATION_TYPE)o.toInt());
}

void GeneralWindowItem::onLatencyDisplayPreferencesChanged(LATENCY_DISPLAY_TYPE l)
{
    comboBoxLatencyDisplay_->setCurrentItem((int)l);
}

void GeneralWindowItem::onLatencyItemChanged(QVariant o)
{
    preferences_->setLatencyDisplay((LATENCY_DISPLAY_TYPE)o.toInt());
}

void GeneralWindowItem::onUpdateChannelPreferencesChanged(const UPDATE_CHANNEL &c)
{
    comboBoxUpdateChannel_->setCurrentItem((int)c);
}

void GeneralWindowItem::onUpdateChannelItemChanged(QVariant o)
{
    preferences_->setUpdateChannel((UPDATE_CHANNEL)o.toInt());
}

void GeneralWindowItem::onBackgroundSettingsChanged(const types::BackgroundSettings &settings)
{
    preferences_->setBackgroundSettings(settings);
}

void GeneralWindowItem::onPreferencesBackgroundSettingsChanged(const types::BackgroundSettings &settings)
{
    backgroundSettingsGroup_->setBackgroundSettings(settings);
}

void GeneralWindowItem::onLanguageChanged()
{
    launchOnStartGroup_->setDescription(tr("Run Windscribe when your device starts."));
    checkBoxLaunchOnStart_->setCaption(tr("Launch on Startup"));
    startMinimizedGroup_->setDescription(tr("Launch Windscribe in a minimized state."));
    checkBoxStartMinimized_->setCaption(tr("Start Minimized"));
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (closeToTrayGroup_) {
        closeToTrayGroup_->setDescription(tr("Windscribe minimizes to system tray and no longer appears in the task bar."));
    }
#else
    if (closeToTrayGroup_) {
        closeToTrayGroup_->setDescription(tr("Windscribe minimizes to menubar and no longer appears in the dock."));
    }
#endif

    if (checkBoxMinimizeAndCloseToTray_) {
        checkBoxMinimizeAndCloseToTray_->setCaption(tr("Close to Tray"));
    }
#if defined(Q_OS_MACOS)
    hideFromDockGroup_->setDescription(tr("Don't show the Windscribe icon in dock."));
    checkBoxHideFromDock_->setCaption(tr("Hide from Dock"));
#endif
#if !defined(Q_OS_LINUX)
    dockedGroup_->setDescription(tr("Pin Windscribe near the system tray or menu bar."));
    checkBoxDockedToTray_->setCaption(tr("Docked"));
#endif
    showNotificationsGroup_->setDescription(tr("Display system-level notifications when connection events occur."));
    checkBoxShowNotifications_->setCaption(tr("Show Notifications"));
    showLocationLoadGroup_->setDescription(tr("Display a location's load. Shorter bars mean lesser load (usage)."));
    checkBoxShowLocationLoad_->setCaption(tr("Show Location Load"));
    locationOrderGroup_->setDescription(tr("Arrange locations alphabetically, geographically, or by latency."));
    comboBoxLocationOrder_->setLabelCaption(tr("Location Order"));
    comboBoxLocationOrder_->setItems(ORDER_LOCATION_TYPE_toList(), preferences_->locationOrder());
    latencyDisplayGroup_->setDescription(tr("Display latency as signal strength bars or in milliseconds."));
    comboBoxLatencyDisplay_->setLabelCaption(tr("Latency Display"));
    comboBoxLatencyDisplay_->setItems(LATENCY_DISPLAY_TYPE_toList(), preferences_->latencyDisplay());
    languageGroup_->setDescription(tr("Localize Windscribe to supported languages."));
    comboBoxLanguage_->setLabelCaption(tr("Language"));
    comboBoxLanguage_->setItems(preferencesHelper_->availableLanguages(), preferences_->language());
    appSkinGroup_->setDescription(tr("Choose between the classic GUI or the \"earless\" alternative GUI."));
    appSkinItem_->setLabelCaption(tr("App Skin"));
    appSkinItem_->setItems(APP_SKIN_toList(), preferences_->appSkin());

#if defined(Q_OS_LINUX)
    trayIconColorGroup_->setDescription(tr("Choose between white and black tray icon."));
    trayIconColorItem_->setLabelCaption(tr("Tray Icon Color"));
    trayIconColorItem_->setItems(TRAY_ICON_COLOR_toList(), preferences_->trayIconColor());
#endif
#if defined(Q_OS_MACOS)
    multiDesktopBehaviorGroup_->setDescription(tr("Select behavior when window is activated with multiple desktops."));
    multiDesktopBehaviorItem_->setLabelCaption(tr("Multi-desktop"));
    multiDesktopBehaviorItem_->setItems(MULTI_DESKTOP_BEHAVIOR_toList(), preferences_->multiDesktopBehavior());
#endif

    backgroundSettingsGroup_->setDescription(tr("Customize the background of the main app screen."));
    renameLocationsGroup_->setDescription(tr("Change location names to your liking."));
    renameLocationsItem_->setTitle(tr("Rename Locations"));
    exportSettingsItem_->setTitle(tr("Export"));
    importSettingsItem_->setTitle(tr("Import"));
    resetLocationsItem_->setTitle(tr("Reset"));
    updateChannelGroup_->setDescription(tr("Choose to receive stable, beta, or experimental builds."));
    comboBoxUpdateChannel_->setLabelCaption(tr("Update Channel"));
    QList<QPair<QString, QVariant>> updateChannelList = UPDATE_CHANNEL_toList();
    for (auto item : updateChannelList) {
        if (item.second == UPDATE_CHANNEL_INTERNAL) {
            updateChannelList.removeOne(item);
            break;
        }
    }
    comboBoxUpdateChannel_->setItems(updateChannelList, preferences_->updateChannel());
    versionInfoItem_->setCaption(tr("Version"));
}

void GeneralWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
}

void GeneralWindowItem::hideOpenPopups()
{
    CommonGraphics::BasePage::hideOpenPopups();

    comboBoxLanguage_      ->hideMenu();
    comboBoxLocationOrder_ ->hideMenu();
    comboBoxLatencyDisplay_->hideMenu();
    comboBoxUpdateChannel_ ->hideMenu();
}

void GeneralWindowItem::onShowLocationLoadClicked(bool b)
{
    preferences_->setShowLocationLoad(b);
}

void GeneralWindowItem::onShowLocationLoadPreferencesChanged(bool b)
{
    checkBoxShowLocationLoad_->setState(b);
}

void GeneralWindowItem::onAppSkinChanged(QVariant value)
{
    preferences_->setAppSkin((APP_SKIN)value.toInt());
}

#if defined(Q_OS_LINUX)
void GeneralWindowItem::onTrayIconColorChanged(QVariant value)
{
    preferences_->setTrayIconColor((TRAY_ICON_COLOR)value.toInt());
}

void GeneralWindowItem::onPreferencesTrayIconColorChanged(QVariant value)
{
    trayIconColorItem_->setCurrentItem(value);
}
#endif

void GeneralWindowItem::onAppSkinPreferencesChanged(APP_SKIN s)
{
    appSkinItem_->setCurrentItem(s);
}

#if defined(Q_OS_MACOS)
void GeneralWindowItem::onMultiDesktopBehaviorChanged(QVariant value)
{
    preferences_->setMultiDesktopBehavior((MULTI_DESKTOP_BEHAVIOR)value.toInt());
}

void GeneralWindowItem::onMultiDesktopBehaviorPreferencesChanged(QVariant value)
{
    multiDesktopBehaviorItem_->setCurrentItem(value);
}
#endif

void GeneralWindowItem::onVersionInfoClicked()
{
#if defined(Q_OS_MACOS)
    // macOS platform name on website is "mac" instead of "macos"
    QString platform = "mac";
#else
    QString platform = Utils::getPlatformName();
#endif
    QDesktopServices::openUrl(QUrl(
        QString("https://%1/changelog/%2")
            .arg(HardcodedSettings::instance().windscribeServerUrl())
            .arg(platform)));
}

void GeneralWindowItem::setLocationNamesImportCompleted()
{
    importSettingsItem_->setLinkIcon(ImageResourcesSvg::instance().getIndependentPixmap("CHECKMARK"));

    // Revert to old icon after 5 seconds
    QTimer::singleShot(5000, [this](){
        importSettingsItem_->setLinkIcon(nullptr);
    });
}

void GeneralWindowItem::onResetLocationNamesClicked()
{
    emit resetLocationNamesClick();

    resetLocationsItem_->setLinkIcon(ImageResourcesSvg::instance().getIndependentPixmap("CHECKMARK"));

    // Revert to old icon after 5 seconds
    QTimer::singleShot(5000, [this](){
        resetLocationsItem_->setLinkIcon(nullptr);
    });
}

} // namespace PreferencesWindow
