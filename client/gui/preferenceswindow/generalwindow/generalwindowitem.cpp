#include "generalwindowitem.h"

#include <QDesktopServices>
#include <QPainter>
#include <QSystemTrayIcon>
#include "utils/hardcodedsettings.h"
#include "backend/persistentstate.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencegroup.h"
#include "utils/logger.h"

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

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    connect(preferences, &Preferences::minimizeAndCloseToTrayChanged, this, &GeneralWindowItem::onMinimizeAndCloseToTrayPreferencesChanged);
#elif defined Q_OS_MAC
    connect(preferences, &Preferences::hideFromDockChanged, this, &GeneralWindowItem::onHideFromDockPreferecesChanged);
#endif

    launchOnStartGroup_ = new PreferenceGroup(this);
    checkBoxLaunchOnStart_ = new CheckBoxItem(launchOnStartGroup_);
    checkBoxLaunchOnStart_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LAUNCH_AT_STARTUP"));
    checkBoxLaunchOnStart_->setState(preferences->isLaunchOnStartup());
    connect(checkBoxLaunchOnStart_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onIsLaunchOnStartupClicked);
    launchOnStartGroup_->addItem(checkBoxLaunchOnStart_);
    addItem(launchOnStartGroup_);

    startMinimizedGroup_ = new PreferenceGroup(this);
    checkBoxStartMinimized_ = new CheckBoxItem(startMinimizedGroup_);
    checkBoxStartMinimized_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/START_MINIMIZED"));
    checkBoxStartMinimized_->setState(preferences->isStartMinimized());
    connect(checkBoxStartMinimized_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onStartMinimizedClicked);
    startMinimizedGroup_->addItem(checkBoxStartMinimized_);
    addItem(startMinimizedGroup_);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        closeToTrayGroup_ = new PreferenceGroup(this);
        checkBoxMinimizeAndCloseToTray_ = new CheckBoxItem(closeToTrayGroup_);
        checkBoxMinimizeAndCloseToTray_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/MINIMIZE_AND_CLOSE_TO_TRAY"));
        checkBoxMinimizeAndCloseToTray_->setState(preferences->isMinimizeAndCloseToTray());
        connect(checkBoxMinimizeAndCloseToTray_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onMinimizeAndCloseToTrayClicked);
        closeToTrayGroup_->addItem(checkBoxMinimizeAndCloseToTray_);
        addItem(closeToTrayGroup_);
    }
#elif defined Q_OS_MAC
    hideFromDockGroup_ = new PreferenceGroup(this);
    checkBoxHideFromDock_ = new CheckBoxItem(hideFromDockGroup_);
    checkBoxHideFromDock_->setState(preferences->isHideFromDock());
    checkBoxHideFromDock_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/HIDE_FROM_DOCK"));
    connect(checkBoxHideFromDock_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onHideFromDockClicked);
    hideFromDockGroup_->addItem(checkBoxHideFromDock_);
    addItem(hideFromDockGroup_);
#endif

    dockedGroup_ = new PreferenceGroup(this);
    checkBoxDockedToTray_ = new CheckBoxItem(dockedGroup_);
    checkBoxDockedToTray_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/DOCKED"));
    checkBoxDockedToTray_->setState(preferences->isDockedToTray());
    connect(checkBoxDockedToTray_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onDockedToTrayChanged);
    dockedGroup_->addItem(checkBoxDockedToTray_);
    addItem(dockedGroup_);

    showNotificationsGroup_ = new PreferenceGroup(this);
    checkBoxShowNotifications_ = new CheckBoxItem(showNotificationsGroup_);
    checkBoxShowNotifications_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SHOW_NOTIFICATIONS"));
    checkBoxShowNotifications_->setState(preferences->isShowNotifications());
    connect(checkBoxShowNotifications_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onIsShowNotificationsClicked);
    showNotificationsGroup_->addItem(checkBoxShowNotifications_);
    addItem(showNotificationsGroup_);

    showLocationLoadGroup_ = new PreferenceGroup(this);
    checkBoxShowLocationLoad_ = new CheckBoxItem(showLocationLoadGroup_);
    checkBoxShowLocationLoad_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LOCATION_LOAD"));
    checkBoxShowLocationLoad_->setState(preferences->isShowLocationLoad());
    connect(checkBoxShowLocationLoad_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onShowLocationLoadClicked);
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

    backgroundSettingsGroup_ = new BackgroundSettingsGroup(this);
    backgroundSettingsGroup_->setBackgroundSettings(preferences->backgroundSettings());
    connect(backgroundSettingsGroup_, &BackgroundSettingsGroup::backgroundSettingsChanged, this, &GeneralWindowItem::onBackgroundSettingsChanged);
    addItem(backgroundSettingsGroup_);

    updateChannelGroup_ = new PreferenceGroup(this,
                                              QString(),
                                              QString("https://%1/features/update-channels").arg(HardcodedSettings::instance().serverUrl()));
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
    if(preferences_)
        preferences_->setStartMinimized(b);
}

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
void GeneralWindowItem::onMinimizeAndCloseToTrayPreferencesChanged(bool b)
{
    checkBoxMinimizeAndCloseToTray_->setState(b);
}

void GeneralWindowItem::onMinimizeAndCloseToTrayClicked(bool b)
{
    preferences_->setMinimizeAndCloseToTray(b);
}

#elif defined Q_OS_MAC
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
    checkBoxDockedToTray_->setState(b);
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
    closeToTrayGroup_->setDescription(tr("Windscribe minimizes to system tray and no longer appears in the task bar."));
    checkBoxMinimizeAndCloseToTray_->setCaption(tr("Close to Tray"));
#elif defined(Q_OS_MAC)
    hideFromDockGroup_->setDescription(tr("Don't show the Windscribe icon in dock."));
    checkBoxHideFromDock_->setCaption(tr("Hide from Dock"));
#endif
    dockedGroup_->setDescription(tr("Pin Windscribe near the system tray or menu bar."));
    checkBoxDockedToTray_->setCaption(tr("Docked"));
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
    backgroundSettingsGroup_->setDescription(tr("Customize the background of the main app screen."));
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

    // qCDebug(LOG_PREFERENCES) << "Hiding General popups";
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

void GeneralWindowItem::onAppSkinPreferencesChanged(APP_SKIN s)
{
    appSkinItem_->setCurrentItem(s);
}

void GeneralWindowItem::onVersionInfoClicked()
{
    QString platform;
#ifdef Q_OS_WIN
    platform = "windows";
#elif defined(Q_OS_MAC)
    platform = "mac";
#else
    platform = LinuxUtils::getLastInstallPlatform();
    if (platform == LinuxUtils::DEB_PLATFORM_NAME) {
        platform = "linux_deb";
    } else if (platform == LinuxUtils::RPM_PLATFORM_NAME) {
        platform = "linux_rpm";
    } else {
        // We don't have a website changelog for zst yet, go to top page instead
        platform = "";
    }
#endif

    QDesktopServices::openUrl(QUrl(
        QString("https://%1/changelog/%2")
            .arg(HardcodedSettings::instance().serverUrl())
            .arg(platform)));
}

} // namespace PreferencesWindow
