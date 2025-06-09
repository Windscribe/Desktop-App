#include "appbackgroundgroup.h"

#include <QPainter>

#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

AppBackgroundGroup::AppBackgroundGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemIsFocusable);

    titleItem_ = new LinkItem(this, LinkItem::LinkType::TEXT_ONLY);
    titleItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/APP_BACKGROUND"));
    addItem(titleItem_);

    comboBoxAspectRatioMode_ = new ComboBoxItem(this);
    connect(comboBoxAspectRatioMode_, &ComboBoxItem::currentItemChanged, this, &AppBackgroundGroup::onBackgroundAspectRatioModeChanged);
    addItem(comboBoxAspectRatioMode_);

    disconnectedComboBox_ = new MixedComboBoxItem(this);
    connect(disconnectedComboBox_, &MixedComboBoxItem::currentItemChanged, this, &AppBackgroundGroup::onDisconnectedComboBoxChanged);
    connect(disconnectedComboBox_, &MixedComboBoxItem::pathChanged, this, &AppBackgroundGroup::onDisconnectedPathChanged);
    addItem(disconnectedComboBox_);

    connectedComboBox_ = new MixedComboBoxItem(this);
    connect(connectedComboBox_, &MixedComboBoxItem::currentItemChanged, this, &AppBackgroundGroup::onConnectedComboBoxChanged);
    connect(connectedComboBox_, &MixedComboBoxItem::pathChanged, this, &AppBackgroundGroup::onConnectedPathChanged);
    addItem(connectedComboBox_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &AppBackgroundGroup::onLanguageChanged);
    onLanguageChanged();
}

void AppBackgroundGroup::onLanguageChanged()
{
    titleItem_->setTitle(tr("App Background"));

    comboBoxAspectRatioMode_->setLabelCaption(tr("Aspect Ratio Mode"));
    comboBoxAspectRatioMode_->setItems(ASPECT_RATIO_MODE_toList(), settings_.aspectRatioMode);

    QList<QPair<QString, QVariant>> secondaryItems;
    secondaryItems << qMakePair(tr("Square"), ":/png/bg/bg1.png");
    secondaryItems << qMakePair(tr("Palm"), ":/png/bg/bg2.png");
    secondaryItems << qMakePair(tr("Ripple"), ":/png/bg/bg3.png");
    secondaryItems << qMakePair(tr("Drip"), ":/png/bg/bg4.png");
    secondaryItems << qMakePair(tr("Snow"), ":/png/bg/bg5.png");
    //secondaryItems << qMakePair(tr("Circuit"), ":/png/bg/bg6.png");

    disconnectedComboBox_->setLabelCaption(tr("When Disconnected"));
    QList<QPair<QString, QVariant>> disconnectedList;
    disconnectedList << qMakePair(tr("Flags"), BACKGROUND_TYPE_COUNTRY_FLAGS);
    disconnectedList << qMakePair(tr("Bundled"), BACKGROUND_TYPE_BUNDLED);
    disconnectedList << qMakePair(tr("None"), BACKGROUND_TYPE_NONE);
    disconnectedList << qMakePair(tr("Custom"), BACKGROUND_TYPE_CUSTOM);
    disconnectedComboBox_->setItems(disconnectedList, settings_.disconnectedBackgroundType);
    disconnectedComboBox_->setCustomValue(BACKGROUND_TYPE_CUSTOM);
    disconnectedComboBox_->setBundledValue(BACKGROUND_TYPE_BUNDLED);
    disconnectedComboBox_->setSecondaryItems(secondaryItems, settings_.backgroundImageDisconnected);
    disconnectedComboBox_->setDialogText(tr("Select an image"), "Images (*.png *.jpg *.gif)");

    connectedComboBox_->setLabelCaption(tr("When Connected"));
    QList<QPair<QString, QVariant>> connectedList;
    connectedList << qMakePair(tr("Flags"), BACKGROUND_TYPE_COUNTRY_FLAGS);
    connectedList << qMakePair(tr("Bundled"), BACKGROUND_TYPE_BUNDLED);
    connectedList << qMakePair(tr("None"), BACKGROUND_TYPE_NONE);
    connectedList << qMakePair(tr("Custom"), BACKGROUND_TYPE_CUSTOM);
    connectedComboBox_->setItems(connectedList, settings_.connectedBackgroundType);
    connectedComboBox_->setCustomValue(BACKGROUND_TYPE_CUSTOM);
    connectedComboBox_->setBundledValue(BACKGROUND_TYPE_BUNDLED);
    connectedComboBox_->setSecondaryItems(secondaryItems, settings_.backgroundImageConnected);
    connectedComboBox_->setDialogText(tr("Select an image"), "Images (*.png *.jpg *.gif)");
}

void AppBackgroundGroup::onDisconnectedComboBoxChanged(const QVariant &v)
{
    bool changed = settings_.disconnectedBackgroundType != (BACKGROUND_TYPE)v.toInt();
    if (changed) {
        settings_.disconnectedBackgroundType = (BACKGROUND_TYPE)v.toInt();
        emit backgroundSettingsChanged(settings_);
    }
}

void AppBackgroundGroup::onDisconnectedPathChanged(const QVariant &v)
{
    bool changed = settings_.backgroundImageDisconnected != v.toString();
    if (changed) {
        settings_.backgroundImageDisconnected = v.toString();
        emit backgroundSettingsChanged(settings_);
    }
}

void AppBackgroundGroup::onConnectedComboBoxChanged(const QVariant &v)
{
    bool changed = settings_.connectedBackgroundType != (BACKGROUND_TYPE)v.toInt();
    if (changed) {
        settings_.connectedBackgroundType = (BACKGROUND_TYPE)v.toInt();
        emit backgroundSettingsChanged(settings_);
    }
}

void AppBackgroundGroup::onConnectedPathChanged(const QVariant &v)
{
    bool changed = settings_.backgroundImageConnected != v.toString();
    if (changed) {
        settings_.backgroundImageConnected = v.toString();
        emit backgroundSettingsChanged(settings_);
    }
}

void AppBackgroundGroup::hideOpenPopups()
{
    comboBoxAspectRatioMode_->hideMenu();
    disconnectedComboBox_->hideMenu();
    connectedComboBox_->hideMenu();
}

void AppBackgroundGroup::setBackgroundSettings(const types::BackgroundSettings &settings)
{
    if (settings_ != settings) {
        comboBoxAspectRatioMode_->setCurrentItem(settings.aspectRatioMode);
        disconnectedComboBox_->setCurrentItem(settings.disconnectedBackgroundType);
        disconnectedComboBox_->setPath(settings.backgroundImageDisconnected);
        connectedComboBox_->setCurrentItem(settings.connectedBackgroundType);
        connectedComboBox_->setPath(settings.backgroundImageConnected);
        settings_ = settings;
    }
}

void AppBackgroundGroup::onBackgroundAspectRatioModeChanged(const QVariant &v)
{
    settings_.aspectRatioMode = (ASPECT_RATIO_MODE)v.toInt();
    emit backgroundSettingsChanged(settings_);
}

void AppBackgroundGroup::hideMenu()
{
    hideOpenPopups();
}

void AppBackgroundGroup::setDescription(const QString &desc, const QString &url)
{
    titleItem_->setDescription(desc, url);
}

} // namespace PreferencesWindow
