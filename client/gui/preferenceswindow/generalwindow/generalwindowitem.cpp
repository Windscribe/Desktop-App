#include "generalwindowitem.h"

#include "../dividerline.h"
#include "utils/protoenumtostring.h"
#include "utils/logger.h"
#include <QPainter>
#include <QSystemTrayIcon>
#include "languagecontroller.h"
#include "../dividerline.h"
#include "languagecontroller.h"
#include "backend/persistentstate.h"

namespace PreferencesWindow {

GeneralWindowItem::GeneralWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : BasePage(parent),
    preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences, SIGNAL(isLaunchOnStartupChanged(bool)), SLOT(onIsLaunchOnStartupPreferencesChanged(bool)));
    connect(preferences, SIGNAL(isAutoConnectChanged(bool)), SLOT(onIsAutoConnectPreferencesChanged(bool)));
    connect(preferences, SIGNAL(isShowNotificationsChanged(bool)), SLOT(onIsShowNotificationsPreferencesChanged(bool)));
    connect(preferences, SIGNAL(isDockedToTrayChanged(bool)), SLOT(onIsDockedToTrayPreferencesChanged(bool)));
    connect(preferences, SIGNAL(languageChanged(QString)), SLOT(onLanguagePreferencesChanged(QString)));
    connect(preferences, SIGNAL(locationOrderChanged(ProtoTypes::OrderLocationType)), SLOT(onLocationOrderPreferencesChanged(ProtoTypes::OrderLocationType)));
    connect(preferences, SIGNAL(latencyDisplayChanged(ProtoTypes::LatencyDisplayType)), SLOT(onLatencyDisplayPreferencesChanged(ProtoTypes::LatencyDisplayType)));
    connect(preferences, SIGNAL(updateChannelChanged(ProtoTypes::UpdateChannel)), SLOT(onUpdateChannelPreferencesChanged(ProtoTypes::UpdateChannel)));
    connect(preferences, SIGNAL(backgroundSettingsChanged(ProtoTypes::BackgroundSettings)), SLOT(onPreferencesBackgroundSettingsChanged(ProtoTypes::BackgroundSettings)));
    connect(preferences, SIGNAL(isStartMinimizedChanged(bool)), SLOT(onStartMinimizedPreferencesChanged(bool)));
    connect(preferences, &Preferences::showLocationLoadChanged, this, &GeneralWindowItem::onShowLocationLoadPreferencesChanged);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    connect(preferences, SIGNAL(minimizeAndCloseToTrayChanged(bool)), SLOT(onMinimizeAndCloseToTrayPreferencesChanged(bool)));
#elif defined Q_OS_MAC
    connect(preferences, SIGNAL(hideFromDockChanged(bool)), SLOT(onHideFromDockPreferecesChanged(bool)));
#endif

    checkBoxLaunchOnStart_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Launch on Startup"), QString());
    checkBoxLaunchOnStart_->setState(preferences->isLaunchOnStartup());
    connect(checkBoxLaunchOnStart_, SIGNAL(stateChanged(bool)), SLOT(onIsLaunchOnStartupClicked(bool)));
    addItem(checkBoxLaunchOnStart_);

    checkBoxAutoConnect_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Auto-Connect"), QString());
    checkBoxAutoConnect_->setState(preferences->isAutoConnect());
    connect(checkBoxAutoConnect_, SIGNAL(stateChanged(bool)), SLOT(onIsAutoConnectClicked(bool)));
    addItem(checkBoxAutoConnect_);

    checkBoxStartMinimized_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("Preferences::CheckBoxStartMinimized", "Start Minimized"), QString());
    checkBoxStartMinimized_->setState(preferences->isStartMinimized());
    connect(checkBoxStartMinimized_, SIGNAL(stateChanged(bool)), SLOT(onStartMinimizedClicked(bool)));
    addItem(checkBoxStartMinimized_);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        checkBoxMinimizeAndCloseToTray_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Minimize and Close to Tray"), QString());
        checkBoxMinimizeAndCloseToTray_->setState(preferences->isMinimizeAndCloseToTray());
        connect(checkBoxMinimizeAndCloseToTray_, SIGNAL(stateChanged(bool)), SLOT(onMinimizeAndCloseToTrayClicked(bool)));
        addItem(checkBoxMinimizeAndCloseToTray_);
    }
#elif defined Q_OS_MAC
    checkBoxHideFromDock_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Hide from dock"), QString());
    checkBoxHideFromDock_->setState(preferences->isHideFromDock());
    connect(checkBoxHideFromDock_, SIGNAL(stateChanged(bool)), SLOT(onHideFromDockClicked(bool)));
    addItem(checkBoxHideFromDock_);
#elif defined Q_OS_LINUX
        //todo linux
        //Q_ASSERT(false);
#endif

    checkBoxShowNotifications_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Show Notifications"), QString());
    checkBoxShowNotifications_->setState(preferences->isShowNotifications());
    connect(checkBoxShowNotifications_, SIGNAL(stateChanged(bool)), SLOT(onIsShowNotificationsClicked(bool)));
    addItem(checkBoxShowNotifications_);

    backgroundSettingsItem_ = new BackgroundSettingsItem(this);
    backgroundSettingsItem_->setBackgroundSettings(preferences->backgroundSettings());
    connect(backgroundSettingsItem_, SIGNAL(backgroundSettingsChanged(ProtoTypes::BackgroundSettings)), SLOT(onBackgroundSettingsChanged(ProtoTypes::BackgroundSettings)));
    addItem(backgroundSettingsItem_);

    checkBoxDockedToTray_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Docked"), QString());
    checkBoxDockedToTray_->setState(preferences->isDockedToTray());
    connect(checkBoxDockedToTray_, SIGNAL(stateChanged(bool)), SLOT(onDockedToTrayChanged(bool)));
    addItem(checkBoxDockedToTray_);

    comboBoxLanguage_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Language"), QString(), 50, Qt::transparent, 0, true);
    QList<QPair<QString, QString> > langList = preferencesHelper->availableLanguages();
    for (auto it = langList.begin(); it != langList.end(); ++it)
    {
        comboBoxLanguage_->addItem(it->first, it->second);
    }
    comboBoxLanguage_->setCurrentItem(langList.begin()->second);
    connect(comboBoxLanguage_, SIGNAL(currentItemChanged(QVariant)), SLOT(onLanguageItemChanged(QVariant)));
    addItem(comboBoxLanguage_);

    comboBoxLocationOrder_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Location Order"), QString(), 50, Qt::transparent, 0, true);

    const QList< QPair<QString, int> > allOrderTypes = ProtoEnumToString::instance().getEnums(ProtoTypes::OrderLocationType_descriptor());
    for (const auto &o : allOrderTypes)
    {
        comboBoxLocationOrder_->addItem(o.first, o.second);
    }
    comboBoxLocationOrder_->setCurrentItem((int)preferences->locationOrder());
    connect(comboBoxLocationOrder_, SIGNAL(currentItemChanged(QVariant)), SLOT(onLocationItemChanged(QVariant)));
    addItem(comboBoxLocationOrder_);

    comboBoxLatencyDisplay_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Latency Display"), QString(), 50, Qt::transparent, 0, true);
    const QList< QPair<QString, int> > allLatencyTypes = ProtoEnumToString::instance().getEnums(ProtoTypes::LatencyDisplayType_descriptor());

    for (const auto &l : allLatencyTypes)
    {
        comboBoxLatencyDisplay_->addItem(l.first, l.second);
    }
    comboBoxLatencyDisplay_->setCurrentItem((int)preferences->latencyDisplay());
    connect(comboBoxLatencyDisplay_, SIGNAL(currentItemChanged(QVariant)), SLOT(onLatencyItemChanged(QVariant)));
    addItem(comboBoxLatencyDisplay_);

    checkBoxShowLocationLoad_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Show Location Load"), QString());
    checkBoxShowLocationLoad_->setState(preferences->isShowLocationLoad());
    connect(checkBoxShowLocationLoad_, &CheckBoxItem::stateChanged, this, &GeneralWindowItem::onShowLocationLoadClicked);
    addItem(checkBoxShowLocationLoad_);

    comboBoxUpdateChannel_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Update Channel"), QString(), 50, Qt::transparent, 0, true);
    const QList< QPair<QString, int> > allUpdateChannelTypes = ProtoEnumToString::instance().getEnums(ProtoTypes::UpdateChannel_descriptor());
    for (const auto &u : allUpdateChannelTypes)
    {
        if (u.first != "Internal") // don't display internal channel -- this will be specified by advanced parameters
        {
            comboBoxUpdateChannel_->addItem(u.first, u.second);
        }
    }
    comboBoxUpdateChannel_->setCurrentItem((int)preferences->updateChannel());
    connect(comboBoxUpdateChannel_, SIGNAL(currentItemChanged(QVariant)), SLOT(onUpdateChannelItemChanged(QVariant)));
    addItem(comboBoxUpdateChannel_);

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    versionInfoItem_ = new VersionInfoItem(this, tr("Version"), preferencesHelper->buildVersion());
    addItem(versionInfoItem_);
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

void GeneralWindowItem::onIsAutoConnectClicked(bool isChecked)
{
    preferences_->setAutoConnect(isChecked);
}

void GeneralWindowItem::onIsAutoConnectPreferencesChanged(bool b)
{
    checkBoxAutoConnect_->setState(b);
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

void GeneralWindowItem::onLocationOrderPreferencesChanged(ProtoTypes::OrderLocationType o)
{
    comboBoxLocationOrder_->setCurrentItem((int)o);
}

void GeneralWindowItem::onLocationItemChanged(QVariant o)
{
    preferences_->setLocationOrder((ProtoTypes::OrderLocationType)o.toInt());
}

void GeneralWindowItem::onLatencyDisplayPreferencesChanged(ProtoTypes::LatencyDisplayType l)
{
    comboBoxLatencyDisplay_->setCurrentItem((int)l);
}

void GeneralWindowItem::onLatencyItemChanged(QVariant o)
{
    preferences_->setLatencyDisplay((ProtoTypes::LatencyDisplayType)o.toInt());
}

void GeneralWindowItem::onUpdateChannelPreferencesChanged(const ProtoTypes::UpdateChannel &c)
{
    comboBoxUpdateChannel_->setCurrentItem((int)c);
}

void GeneralWindowItem::onUpdateChannelItemChanged(QVariant o)
{
    preferences_->setUpdateChannel((ProtoTypes::UpdateChannel)o.toInt());
}

void GeneralWindowItem::onBackgroundSettingsChanged(const ProtoTypes::BackgroundSettings &settings)
{
    preferences_->setBackgroundSettings(settings);
}

void GeneralWindowItem::onPreferencesBackgroundSettingsChanged(const ProtoTypes::BackgroundSettings &settings)
{
    backgroundSettingsItem_->setBackgroundSettings(settings);
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
    BasePage::updateScaling();
}

void GeneralWindowItem::hideOpenPopups()
{
    BasePage::hideOpenPopups();

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

} // namespace PreferencesWindow
