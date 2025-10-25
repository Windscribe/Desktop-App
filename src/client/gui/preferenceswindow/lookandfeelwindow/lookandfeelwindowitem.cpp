#include "lookandfeelwindowitem.h"

#include <QDesktopServices>
#include <QPainter>
#include <QSystemTrayIcon>
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

LookAndFeelWindowItem::LookAndFeelWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : CommonGraphics::BasePage(parent),
    preferences_(preferences), preferencesHelper_(preferencesHelper)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    connect(preferences, &Preferences::appSkinChanged, this, &LookAndFeelWindowItem::onAppSkinPreferencesChanged);
    connect(preferences, &Preferences::backgroundSettingsChanged, this, &LookAndFeelWindowItem::onPreferencesBackgroundSettingsChanged);
    connect(preferences, &Preferences::soundSettingsChanged, this, &LookAndFeelWindowItem::onPreferencesSoundSettingsChanged);
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    connect(preferences, &Preferences::trayIconColorChanged, this, &LookAndFeelWindowItem::onPreferencesTrayIconColorChanged);
#endif

    appBackgroundGroup_ = new AppBackgroundGroup(this);
    appBackgroundGroup_->setBackgroundSettings(preferences->backgroundSettings());
    connect(appBackgroundGroup_, &AppBackgroundGroup::backgroundSettingsChanged, this, &LookAndFeelWindowItem::onBackgroundSettingsChanged);
    addItem(appBackgroundGroup_);

    soundsGroup_ = new SoundsGroup(this);
    soundsGroup_->setSoundSettings(preferences->soundSettings());
    connect(soundsGroup_, &SoundsGroup::soundSettingsChanged, this, &LookAndFeelWindowItem::onSoundSettingsChanged);
    addItem(soundsGroup_);

    renameLocationsGroup_ = new PreferenceGroup(this);
    renameLocationsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::TEXT_ONLY);
    renameLocationsItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/RENAME_LOCATIONS"));
    renameLocationsGroup_->addItem(renameLocationsItem_);

    exportSettingsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(exportSettingsItem_, &LinkItem::clicked, this, &LookAndFeelWindowItem::exportLocationNamesClick);
    renameLocationsGroup_->addItem(exportSettingsItem_);

    importSettingsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(importSettingsItem_, &LinkItem::clicked, this, &LookAndFeelWindowItem::importLocationNamesClick);
    renameLocationsGroup_->addItem(importSettingsItem_);

    resetLocationsItem_ = new LinkItem(renameLocationsGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(resetLocationsItem_, &LinkItem::clicked, this, &LookAndFeelWindowItem::onResetLocationNamesClicked);
    renameLocationsGroup_->addItem(resetLocationsItem_);

    addItem(renameLocationsGroup_);

    appSkinGroup_ = new PreferenceGroup(this);
    appSkinItem_ = new ComboBoxItem(appSkinGroup_);
    appSkinItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/APP_SKIN"));
    connect(appSkinItem_, &ComboBoxItem::currentItemChanged, this, &LookAndFeelWindowItem::onAppSkinChanged);
    appSkinGroup_->addItem(appSkinItem_);
    addItem(appSkinGroup_);

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    trayIconColorGroup_ = new PreferenceGroup(this);
    trayIconColorItem_ = new ComboBoxItem(trayIconColorGroup_);
    trayIconColorItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TRAY_ICON_COLOR"));
    connect(trayIconColorItem_, &ComboBoxItem::currentItemChanged, this, &LookAndFeelWindowItem::onTrayIconColorChanged);
    trayIconColorGroup_->addItem(trayIconColorItem_);
    addItem(trayIconColorGroup_);
#endif

    // Populate combo boxes and other text
    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &LookAndFeelWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString LookAndFeelWindowItem::caption() const
{
    return tr("Look & Feel");
}

void LookAndFeelWindowItem::onBackgroundSettingsChanged(const types::BackgroundSettings &settings)
{
    preferences_->setBackgroundSettings(settings);
}

void LookAndFeelWindowItem::onPreferencesBackgroundSettingsChanged(const types::BackgroundSettings &settings)
{
    appBackgroundGroup_->setBackgroundSettings(settings);
}

void LookAndFeelWindowItem::onSoundSettingsChanged(const types::SoundSettings &settings)
{
    preferences_->setSoundSettings(settings);
}

void LookAndFeelWindowItem::onPreferencesSoundSettingsChanged(const types::SoundSettings &settings)
{
    soundsGroup_->setSoundSettings(settings);
}

void LookAndFeelWindowItem::onLanguageChanged()
{
    appSkinItem_->setDescription(tr("Choose between the classic GUI or the \"earless\" alternative GUI."));
    appSkinItem_->setLabelCaption(tr("App Skin"));
    appSkinItem_->setItems(APP_SKIN_toList(), preferences_->appSkin());

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    trayIconColorItem_->setDescription(tr("Choose between white and black tray icon."));
    trayIconColorItem_->setLabelCaption(tr("Tray Icon Colour"));
    trayIconColorItem_->setItems(TRAY_ICON_COLOR_toList(), preferences_->trayIconColor());
#endif

    appBackgroundGroup_->setDescription(tr("Customize the background of the main app screen."));
    soundsGroup_->setDescription(tr("Choose sounds to play when connection events occur."));

    renameLocationsItem_->setDescription(tr("Change location names to your liking."));
    renameLocationsItem_->setTitle(tr("Rename Locations"));
    exportSettingsItem_->setTitle(tr("Export"));
    importSettingsItem_->setTitle(tr("Import"));
    resetLocationsItem_->setTitle(tr("Reset"));
}

void LookAndFeelWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
}

void LookAndFeelWindowItem::hideOpenPopups()
{
    CommonGraphics::BasePage::hideOpenPopups();

    appBackgroundGroup_->hideMenu();
    soundsGroup_->hideMenu();
}

void LookAndFeelWindowItem::onAppSkinChanged(QVariant value)
{
    preferences_->setAppSkin((APP_SKIN)value.toInt());
}

void LookAndFeelWindowItem::onAppSkinPreferencesChanged(APP_SKIN s)
{
    appSkinItem_->setCurrentItem(s);
}

void LookAndFeelWindowItem::setLocationNamesImportCompleted()
{
    importSettingsItem_->setLinkIcon(ImageResourcesSvg::instance().getIndependentPixmap("CHECKMARK"));

    // Revert to old icon after 5 seconds
    QTimer::singleShot(5000, [this](){
        importSettingsItem_->setLinkIcon(nullptr);
    });
}

void LookAndFeelWindowItem::onResetLocationNamesClicked()
{
    emit resetLocationNamesClick();

    resetLocationsItem_->setLinkIcon(ImageResourcesSvg::instance().getIndependentPixmap("CHECKMARK"));

    // Revert to old icon after 5 seconds
    QTimer::singleShot(5000, [this](){
        resetLocationsItem_->setLinkIcon(nullptr);
    });
}

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
void LookAndFeelWindowItem::onTrayIconColorChanged(QVariant value)
{
    preferences_->setTrayIconColor((TRAY_ICON_COLOR)value.toInt());
}

void LookAndFeelWindowItem::onPreferencesTrayIconColorChanged(QVariant value)
{
    trayIconColorItem_->setCurrentItem(value);
}
#endif

} // namespace PreferencesWindow
