#include "generalwindowitem.h"

#include <QPainter>
#include <QSystemTrayIcon>
#include "utils/hardcodedsettings.h"
#include "backend/persistentstate.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencegroup.h"
#include "utils/logger.h"

namespace PreferencesWindow {

GeneralWindowItem::GeneralWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : CommonGraphics::BasePage(parent),
    preferences_(preferences)
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

    launchOnStartGroup_ = new PreferenceGroup(this, tr("Run Windscribe when your device starts."));
    checkBoxLaunchOnStart_ = new CheckBoxItem(launchOnStartGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Launch on Startup"), QString());
    checkBoxLaunchOnStart_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LAUNCH_AT_STARTUP"));
    checkBoxLaunchOnStart_->setState(preferences->isLaunchOnStartup());
    connect(checkBoxLaunchOnStart_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onIsLaunchOnStartupClicked);
    launchOnStartGroup_->addItem(checkBoxLaunchOnStart_);
    addItem(launchOnStartGroup_);

    startMinimizedGroup_ = new PreferenceGroup(this, tr("Launch Windscribe in a minimized state."));
    checkBoxStartMinimized_ = new CheckBoxItem(startMinimizedGroup_, QT_TRANSLATE_NOOP("Preferences::CheckBoxStartMinimized", "Start Minimized"), QString());
    checkBoxStartMinimized_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/START_MINIMIZED"));
    checkBoxStartMinimized_->setState(preferences->isStartMinimized());
    connect(checkBoxStartMinimized_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onStartMinimizedClicked);
    startMinimizedGroup_->addItem(checkBoxStartMinimized_);
    addItem(startMinimizedGroup_);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        closeToTrayGroup_ = new PreferenceGroup(this, tr("Windscribe minimizes to system tray and no longer appears in the task bar."));
        checkBoxMinimizeAndCloseToTray_ = new CheckBoxItem(closeToTrayGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Close to Tray"), QString());
        checkBoxMinimizeAndCloseToTray_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/MINIMIZE_AND_CLOSE_TO_TRAY"));
        checkBoxMinimizeAndCloseToTray_->setState(preferences->isMinimizeAndCloseToTray());
        connect(checkBoxMinimizeAndCloseToTray_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onMinimizeAndCloseToTrayClicked);
        closeToTrayGroup_->addItem(checkBoxMinimizeAndCloseToTray_);
        addItem(closeToTrayGroup_);
    }
#elif defined Q_OS_MAC
    hideFromDockGroup_ = new PreferenceGroup(this, tr("Don't show the Windscribe icon in dock."), "");
    checkBoxHideFromDock_ = new CheckBoxItem(hideFromDockGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Hide from dock"), QString());
    checkBoxHideFromDock_->setState(preferences->isHideFromDock());
    checkBoxHideFromDock_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/HIDE_FROM_DOCK"));
    connect(checkBoxHideFromDock_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onHideFromDockClicked);
    hideFromDockGroup_->addItem(checkBoxHideFromDock_);
    addItem(hideFromDockGroup_);
#endif
    dockedGroup_ = new PreferenceGroup(this, tr("Pin Windscribe near the system tray or menu bar."));
    checkBoxDockedToTray_ = new CheckBoxItem(dockedGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Docked"), QString());
    checkBoxDockedToTray_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/DOCKED"));
    checkBoxDockedToTray_->setState(preferences->isDockedToTray());
    connect(checkBoxDockedToTray_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onDockedToTrayChanged);
    dockedGroup_->addItem(checkBoxDockedToTray_);
    addItem(dockedGroup_);

    showNotificationsGroup_ = new PreferenceGroup(this, tr("Display system-level notifications when connection events occur."));
    checkBoxShowNotifications_ = new CheckBoxItem(showNotificationsGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Show Notifications"), QString());
    checkBoxShowNotifications_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SHOW_NOTIFICATIONS"));
    checkBoxShowNotifications_->setState(preferences->isShowNotifications());
    connect(checkBoxShowNotifications_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onIsShowNotificationsClicked);
    showNotificationsGroup_->addItem(checkBoxShowNotifications_);
    addItem(showNotificationsGroup_);

    showLocationLoadGroup_ = new PreferenceGroup(this, tr("Display a location's load. Shorter bars mean lesser load (usage)."));
    checkBoxShowLocationLoad_ = new CheckBoxItem(showLocationLoadGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Show Location Load"), QString());
    checkBoxShowLocationLoad_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LOCATION_LOAD"));
    checkBoxShowLocationLoad_->setState(preferences->isShowLocationLoad());
    connect(checkBoxShowLocationLoad_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onShowLocationLoadClicked);
    showLocationLoadGroup_->addItem(checkBoxShowLocationLoad_);
    addItem(showLocationLoadGroup_);

    locationOrderGroup_ = new PreferenceGroup(this, tr("Arrange locations alphabetically, geographically, or by latency."));
    comboBoxLocationOrder_ = new ComboBoxItem(locationOrderGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Location Order"), QString());
    comboBoxLocationOrder_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LOCATION_ORDER"));
    const QList< QPair<QString, int> > allOrderTypes = ORDER_LOCATION_TYPE_toList();
    for (const auto o : allOrderTypes)
    {
        comboBoxLocationOrder_->addItem(o.first, o.second);
    }
    comboBoxLocationOrder_->setCurrentItem((int)preferences->locationOrder());
    connect(comboBoxLocationOrder_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onLocationItemChanged);
    locationOrderGroup_->addItem(comboBoxLocationOrder_);
    addItem(locationOrderGroup_);

    latencyDisplayGroup_ = new PreferenceGroup(this, tr("Display latency as signal strength bars or in milliseconds."));
    comboBoxLatencyDisplay_ = new ComboBoxItem(latencyDisplayGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Latency Display"), QString());
    comboBoxLatencyDisplay_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LATENCY_DISPLAY"));
    const QList< QPair<QString, int> > allLatencyTypes = LATENCY_DISPLAY_TYPE_toList();
    for (const auto l : allLatencyTypes)
    {
        comboBoxLatencyDisplay_->addItem(l.first, l.second);
    }
    comboBoxLatencyDisplay_->setCurrentItem((int)preferences->latencyDisplay());
    connect(comboBoxLatencyDisplay_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onLatencyItemChanged);
    latencyDisplayGroup_->addItem(comboBoxLatencyDisplay_);
    addItem(latencyDisplayGroup_);

    languageGroup_ = new PreferenceGroup(this, tr("Localize Windscribe to supported languages."));
    comboBoxLanguage_ = new ComboBoxItem(languageGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Language"), QString());
    comboBoxLanguage_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/LANGUAGE"));
    QList<QPair<QString, QString> > langList = preferencesHelper->availableLanguages();
    for (auto it = langList.begin(); it != langList.end(); ++it)
    {
        comboBoxLanguage_->addItem(it->first, it->second);
    }
    comboBoxLanguage_->setCurrentItem(langList.begin()->second);
    connect(comboBoxLanguage_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onLanguageItemChanged);
    languageGroup_->addItem(comboBoxLanguage_);
    addItem(languageGroup_);

    appSkinGroup_ = new PreferenceGroup(this, tr("Choose between the classic GUI or the \"earless\" alternative GUI."));
    appSkinItem_ = new ComboBoxItem(appSkinGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "App Skin"), QString());
    appSkinItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/APP_SKIN"));
    const QList< QPair<QString, int> > allAppSkins = APP_SKIN_toList();
    for (const auto l : allAppSkins)
    {
        appSkinItem_->addItem(l.first, l.second);
    }
    appSkinItem_->setCurrentItem(preferences->appSkin());
    connect(appSkinItem_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onAppSkinChanged);
    appSkinGroup_->addItem(appSkinItem_);
    addItem(appSkinGroup_);

    backgroundSettingsGroup_ = new BackgroundSettingsGroup(this, tr("Customize the background of the main app screen."));
    backgroundSettingsGroup_->setBackgroundSettings(preferences->backgroundSettings());
    connect(backgroundSettingsGroup_, &BackgroundSettingsGroup::backgroundSettingsChanged, this, &GeneralWindowItem::onBackgroundSettingsChanged);
    addItem(backgroundSettingsGroup_);

    updateChannelGroup_ = new PreferenceGroup(this,
                                              tr("Choose to receive stable, beta, or experimental builds."),
                                              QString("https://%1/features/update-channel").arg(HardcodedSettings::instance().serverUrl()));
    comboBoxUpdateChannel_ = new ComboBoxItem(updateChannelGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Update Channel"), QString());
    comboBoxUpdateChannel_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/UPDATE_CHANNEL"));
    const QList< QPair<QString, int> > allUpdateChannelTypes = UPDATE_CHANNEL_toList();
    for (const auto u : allUpdateChannelTypes)
    {
        if (u.first != "Internal") // don't display internal channel -- this will be specified by advanced parameters
        {
            comboBoxUpdateChannel_->addItem(u.first, u.second);
        }
    }
    comboBoxUpdateChannel_->setCurrentItem((int)preferences->updateChannel());
    connect(comboBoxUpdateChannel_, &ComboBoxItem::currentItemChanged, this, &GeneralWindowItem::onUpdateChannelItemChanged);
    updateChannelGroup_->addItem(comboBoxUpdateChannel_);
    addItem(updateChannelGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &GeneralWindowItem::onLanguageChanged);

    versionGroup_ = new PreferenceGroup(this);
    versionGroup_->setDrawBackground(false);
    versionInfoItem_ = new VersionInfoItem(versionGroup_, tr("Version"), preferencesHelper->buildVersion());
    versionGroup_->addItem(versionInfoItem_);
    addItem(versionGroup_);
}

QString GeneralWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "General");
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
    /*QVariant order = comboBoxLocationOrder_->currentItem();
    QVariant latency = comboBoxLatencyDisplay_->currentItem();
    QVariant update = comboBoxUpdateChannel_->currentItem();

    comboBoxLocationOrder_->clear();
    comboBoxLatencyDisplay_->clear();
    comboBoxUpdateChannel_->clear();

    const QList<OrderLocationType> allOrderTypes = OrderLocationType::allAvailableTypes();
    for (const OrderLocationType &o : allOrderTypes)
    {
        comboBoxLocationOrder_->addItem(o.toString(), (int)o.type());
    }
    comboBoxLocationOrder_->setCurrentItem(order.toInt());

    const QList<LatencyDisplayType> allLatencyTypes = LatencyDisplayType::allAvailableTypes();
    for (const LatencyDisplayType &l : allLatencyTypes)
    {
        comboBoxLatencyDisplay_->addItem(l.toString(), (int)l.type());
    }
    comboBoxLatencyDisplay_->setCurrentItem(latency.toInt());

    const QList<UpdateChannelType> allUpdateChannelTypes = UpdateChannelType::allAvailableTypes();
    for (const UpdateChannelType &u : allUpdateChannelTypes)
    {
        comboBoxUpdateChannel_->addItem(u.toString(), (int)u.type());
    }
    comboBoxUpdateChannel_->setCurrentItem(update.toInt());*/
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

} // namespace PreferencesWindow
