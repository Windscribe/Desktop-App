#include "backgroundsettingsgroup.h"

#include <QPainter>

#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

BackgroundSettingsGroup::BackgroundSettingsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemIsFocusable);

    comboBoxMode_ = new ComboBoxItem(this);
    comboBoxMode_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/APP_BACKGROUND"));
    connect(comboBoxMode_, &ComboBoxItem::currentItemChanged, this, &BackgroundSettingsGroup::onBackgroundModeChanged);
    addItem(comboBoxMode_);

    imageItemDisconnected_ = new SelectImageItem(this, tr("Disconnected"));
    connect(imageItemDisconnected_, &SelectImageItem::pathChanged, this, &BackgroundSettingsGroup::onDisconnectedPathChanged);
    addItem(imageItemDisconnected_);

    imageItemConnected_ = new SelectImageItem(this, tr("Connected"));
    connect(imageItemConnected_, &SelectImageItem::pathChanged, this, &BackgroundSettingsGroup::onConnectedPathChanged);
    addItem(imageItemConnected_);

    hideItems(indexOf(imageItemDisconnected_), indexOf(imageItemConnected_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &BackgroundSettingsGroup::onLanguageChanged);
    onLanguageChanged();
}

void BackgroundSettingsGroup::onLanguageChanged()
{
    comboBoxMode_->setLabelCaption(tr("Background"));
    QList<QPair<QString, QVariant>> list;
    list << qMakePair(tr("Country Flags"), BACKGROUND_TYPE_COUNTRY_FLAGS);
    list << qMakePair(tr("None"), BACKGROUND_TYPE_NONE);
    list << qMakePair(tr("Custom"), BACKGROUND_TYPE_CUSTOM);
    comboBoxMode_->setItems(list, settings_.backgroundType);

    imageItemDisconnected_->setCaption(tr("Disconnected"));
    imageItemConnected_->setCaption(tr("Connected"));
}

void BackgroundSettingsGroup::onDisconnectedPathChanged(const QString &path)
{
    settings_.backgroundImageDisconnected = path;
    emit backgroundSettingsChanged(settings_);
}

void BackgroundSettingsGroup::onConnectedPathChanged(const QString &path)
{
    settings_.backgroundImageConnected = path;
    emit backgroundSettingsChanged(settings_);
}

void BackgroundSettingsGroup::hideOpenPopups()
{
    comboBoxMode_->hideMenu();
}

void BackgroundSettingsGroup::setBackgroundSettings(const types::BackgroundSettings &settings)
{
    if (settings_ != settings) {
        settings_ = settings;
        comboBoxMode_->setCurrentItem(settings_.backgroundType);
        imageItemDisconnected_->setPath(settings_.backgroundImageDisconnected);
        imageItemConnected_->setPath(settings_.backgroundImageConnected);
        updateMode();
    }
}

void BackgroundSettingsGroup::onBackgroundModeChanged(QVariant v)
{
    if (settings_.backgroundType != v.toInt()) {
        settings_.backgroundType = (BACKGROUND_TYPE)v.toInt();
        updateMode();
        emit backgroundSettingsChanged(settings_);
    }
}

void BackgroundSettingsGroup::updateMode()
{
    if (settings_.backgroundType == BACKGROUND_TYPE_CUSTOM) {
        showItems(indexOf(imageItemDisconnected_));
        showItems(indexOf(imageItemConnected_));
    }
    else {
        hideItems(indexOf(imageItemDisconnected_));
        hideItems(indexOf(imageItemConnected_));
    }
}

} // namespace PreferencesWindow
