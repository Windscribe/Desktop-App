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
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    connect(preferences, &Preferences::isLaunchOnStartupChanged, this, &GeneralWindowItem::onIsLaunchOnStartupPreferencesChanged);
    connect(preferences, &Preferences::isShowNotificationsChanged, this, &GeneralWindowItem::onIsShowNotificationsPreferencesChanged);
    connect(preferences, &Preferences::isDockedToTrayChanged, this, &GeneralWindowItem::onIsDockedToTrayPreferencesChanged);
    connect(preferences, &Preferences::languageChanged, this, &GeneralWindowItem::onLanguagePreferencesChanged);
    connect(preferences, &Preferences::locationOrderChanged, this, &GeneralWindowItem::onLocationOrderPreferencesChanged);
    connect(preferences, &Preferences::updateChannelChanged, this, &GeneralWindowItem::onUpdateChannelPreferencesChanged);
    connect(preferences, &Preferences::isStartMinimizedChanged, this, &GeneralWindowItem::onStartMinimizedPreferencesChanged);
    connect(preferences, &Preferences::showLocationLoadChanged, this, &GeneralWindowItem::onShowLocationLoadPreferencesChanged);
    connect(preferences, &Preferences::minimizeAndCloseToTrayChanged, this, &GeneralWindowItem::onMinimizeAndCloseToTrayPreferencesChanged);
#if defined Q_OS_MACOS
    connect(preferences, &Preferences::hideFromDockChanged, this, &GeneralWindowItem::onHideFromDockPreferecesChanged);
    connect(preferences, &Preferences::multiDesktopBehaviorChanged, this, &GeneralWindowItem::onMultiDesktopBehaviorPreferencesChanged);
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

#if defined(Q_OS_MACOS)
    multiDesktopBehaviorGroup_ = new PreferenceGroup(this);
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

    updateChannelGroup_ = new PreferenceGroup(this);
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

void GeneralWindowItem::onUpdateChannelPreferencesChanged(const UPDATE_CHANNEL &c)
{
    comboBoxUpdateChannel_->setCurrentItem((int)c);
}

void GeneralWindowItem::onUpdateChannelItemChanged(QVariant o)
{
    preferences_->setUpdateChannel((UPDATE_CHANNEL)o.toInt());
}

void GeneralWindowItem::onLanguageChanged()
{
    checkBoxLaunchOnStart_->setDescription(tr("Run Windscribe when your device starts."));
    checkBoxLaunchOnStart_->setCaption(tr("Launch on Startup"));
    checkBoxStartMinimized_->setDescription(tr("Launch Windscribe in a minimized state."));
    checkBoxStartMinimized_->setCaption(tr("Start Minimized"));
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (closeToTrayGroup_) {
        checkBoxMinimizeAndCloseToTray_->setDescription(tr("Windscribe minimizes to system tray and no longer appears in the task bar."));
    }
#else
    if (closeToTrayGroup_) {
        checkBoxMinimizeAndCloseToTray_->setDescription(tr("Windscribe minimizes to menubar and no longer appears in the dock."));
    }
#endif

    if (checkBoxMinimizeAndCloseToTray_) {
        checkBoxMinimizeAndCloseToTray_->setCaption(tr("Close to Tray"));
    }
#if defined(Q_OS_MACOS)
    checkBoxHideFromDock_->setDescription(tr("Don't show the Windscribe icon in dock."));
    checkBoxHideFromDock_->setCaption(tr("Hide from Dock"));
#endif
#if !defined(Q_OS_LINUX)
    checkBoxDockedToTray_->setDescription(tr("Pin Windscribe near the system tray or menu bar."));
    checkBoxDockedToTray_->setCaption(tr("Docked"));
#endif
    checkBoxShowNotifications_->setDescription(tr("Display system-level notifications when connection events occur."));
    checkBoxShowNotifications_->setCaption(tr("Show Notifications"));
    checkBoxShowLocationLoad_->setDescription(tr("Display a location's load. Shorter bars mean lesser load (usage)."));
    checkBoxShowLocationLoad_->setCaption(tr("Show Location Load"));
    comboBoxLocationOrder_->setDescription(tr("Arrange locations alphabetically, geographically, or by latency."));
    comboBoxLocationOrder_->setLabelCaption(tr("Location Order"));
    comboBoxLocationOrder_->setItems(ORDER_LOCATION_TYPE_toList(), preferences_->locationOrder());
    comboBoxLanguage_->setDescription(tr("Localize Windscribe to supported languages."));
    comboBoxLanguage_->setLabelCaption(tr("Language"));
    comboBoxLanguage_->setItems(preferencesHelper_->availableLanguages(), preferences_->language());

#if defined(Q_OS_MACOS)
    multiDesktopBehaviorItem_->setDescription(tr("Select behaviour when window is activated with multiple desktops."),
                                              "https://github.com/Windscribe/Desktop-App/wiki/macOS-Multi%E2%80%90desktop-preference");
    multiDesktopBehaviorItem_->setLabelCaption(tr("Multi-desktop"));
    multiDesktopBehaviorItem_->setItems(MULTI_DESKTOP_BEHAVIOR_toList(), preferences_->multiDesktopBehavior());
#endif

    comboBoxUpdateChannel_->setDescription(tr("Choose to receive stable, beta, or experimental builds."),
                                           QString("https://%1/features/update-channels").arg(HardcodedSettings::instance().windscribeServerUrl()));
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

} // namespace PreferencesWindow
