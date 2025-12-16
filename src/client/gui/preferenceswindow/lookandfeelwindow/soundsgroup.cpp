#include "soundsgroup.h"

#include <QDirIterator>
#include <QPainter>

#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

SoundsGroup::SoundsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemIsFocusable);

    titleItem_ = new LinkItem(this, LinkItem::LinkType::TEXT_ONLY);
    titleItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SOUND_NOTIFICATIONS"));
    addItem(titleItem_);

    disconnectedComboBox_ = new MixedComboBoxItem(this);
    connect(disconnectedComboBox_, &MixedComboBoxItem::currentItemChanged, this, &SoundsGroup::onDisconnectedComboBoxChanged);
    connect(disconnectedComboBox_, &MixedComboBoxItem::pathChanged, this, &SoundsGroup::onDisconnectedPathChanged);
    addItem(disconnectedComboBox_);

    connectedComboBox_ = new MixedComboBoxItem(this);
    connect(connectedComboBox_, &MixedComboBoxItem::currentItemChanged, this, &SoundsGroup::onConnectedComboBoxChanged);
    connect(connectedComboBox_, &MixedComboBoxItem::pathChanged, this, &SoundsGroup::onConnectedPathChanged);
    addItem(connectedComboBox_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SoundsGroup::onLanguageChanged);
    onLanguageChanged();
}

QStringList SoundsGroup::enumerateSounds()
{
    if (!soundNames_.isEmpty()) {
        return soundNames_;
    }

    QSet<QString> sounds;
    QDirIterator it(":/sounds/");
    while (it.hasNext()) {
        QString path = it.next();
        sounds << displayName(path);
    }

    soundNames_ = QStringList(sounds.values());
    soundNames_.sort();
    return soundNames_;
}

QString SoundsGroup::displayName(const QString &sound) const
{
    QString filename = sound.split("/").last();
    QString name = filename.left(filename.lastIndexOf("_")).replace("_", " ");
    return name;
}

QString SoundsGroup::filename(const QString &sound, bool connected) const
{
    return QString(":/sounds/%1_%2.mp3").arg(QString(sound).replace(" ", "_"), connected ? "on" : "off");
}

void SoundsGroup::onLanguageChanged()
{
    titleItem_->setTitle(tr("Sound Notifications"));

    QList<QPair<QString, QVariant>> secondaryItemsDisconnected;
    QStringList sounds = enumerateSounds();
    // Disconnected list does not have a Fart (Deluxe) sound
    sounds.removeAll("Fart (Deluxe)");
    for (const auto &sound : sounds) {
        secondaryItemsDisconnected << qMakePair(displayName(sound), filename(sound, false));
    }

    disconnectedComboBox_->setLabelCaption(tr("When Disconnected"));
    QList<QPair<QString, QVariant>> disconnectedList;
    disconnectedList << qMakePair(tr("None"), SOUND_NOTIFICATION_TYPE_NONE);
    disconnectedList << qMakePair(tr("Bundled"), SOUND_NOTIFICATION_TYPE_BUNDLED);
    disconnectedList << qMakePair(tr("Custom"), SOUND_NOTIFICATION_TYPE_CUSTOM);
    disconnectedComboBox_->setItems(disconnectedList, settings_.disconnectedSoundType);
    disconnectedComboBox_->setCustomValue(SOUND_NOTIFICATION_TYPE_CUSTOM);
    disconnectedComboBox_->setBundledValue(SOUND_NOTIFICATION_TYPE_BUNDLED);
    disconnectedComboBox_->setSecondaryItems(secondaryItemsDisconnected, settings_.disconnectedSoundPath);
    disconnectedComboBox_->setDialogText(tr("Select a sound"), "Sounds (*.wav *.mp3)");

    QList<QPair<QString, QVariant>> secondaryItemsConnected;
    for (const auto &sound : enumerateSounds()) {
        secondaryItemsConnected << qMakePair(displayName(sound), filename(sound, true));
    }

    connectedComboBox_->setLabelCaption(tr("When Connected"));
    QList<QPair<QString, QVariant>> connectedList;
    connectedList << qMakePair(tr("None"), SOUND_NOTIFICATION_TYPE_NONE);
    connectedList << qMakePair(tr("Bundled"), SOUND_NOTIFICATION_TYPE_BUNDLED);
    connectedList << qMakePair(tr("Custom"), SOUND_NOTIFICATION_TYPE_CUSTOM);
    connectedComboBox_->setItems(connectedList, settings_.connectedSoundType);
    connectedComboBox_->setCustomValue(SOUND_NOTIFICATION_TYPE_CUSTOM);
    connectedComboBox_->setBundledValue(SOUND_NOTIFICATION_TYPE_BUNDLED);
    connectedComboBox_->setSecondaryItems(secondaryItemsConnected, settings_.connectedSoundPath);
    connectedComboBox_->setDialogText(tr("Select a sound"), "Sounds (*.wav *.mp3)");
}

void SoundsGroup::onDisconnectedComboBoxChanged(const QVariant &v)
{
    bool changed = settings_.disconnectedSoundType != (SOUND_NOTIFICATION_TYPE)v.toInt();
    if (changed) {
        settings_.disconnectedSoundType = (SOUND_NOTIFICATION_TYPE)v.toInt();
        emit soundSettingsChanged(settings_);
    }
}

void SoundsGroup::onDisconnectedPathChanged(const QVariant &v)
{
    bool changed = settings_.disconnectedSoundPath != v.toString();
    if (changed) {
        settings_.disconnectedSoundPath = v.toString();
        emit soundSettingsChanged(settings_);
    }
}

void SoundsGroup::onConnectedComboBoxChanged(const QVariant &v)
{
    bool changed = settings_.connectedSoundType != (SOUND_NOTIFICATION_TYPE)v.toInt();
    if (changed) {
        settings_.connectedSoundType = (SOUND_NOTIFICATION_TYPE)v.toInt();
        emit soundSettingsChanged(settings_);
    }
}

void SoundsGroup::onConnectedPathChanged(const QVariant &v)
{
    bool changed = settings_.connectedSoundPath != v.toString();
    if (changed) {
        settings_.connectedSoundPath = v.toString();
        emit soundSettingsChanged(settings_);
    }
}

void SoundsGroup::hideOpenPopups()
{
    disconnectedComboBox_->hideMenu();
    connectedComboBox_->hideMenu();
}

void SoundsGroup::setSoundSettings(const types::SoundSettings &settings)
{
    if (settings_ != settings) {
        disconnectedComboBox_->setCurrentItem(settings.disconnectedSoundType);
        disconnectedComboBox_->setPath(settings.disconnectedSoundPath);
        connectedComboBox_->setCurrentItem(settings.connectedSoundType);
        connectedComboBox_->setPath(settings.connectedSoundPath);
        settings_ = settings;
    }
}

void SoundsGroup::hideMenu()
{
    hideOpenPopups();
}

void SoundsGroup::setDescription(const QString &desc, const QString &url)
{
    titleItem_->setDescription(desc, url);
}

} // namespace PreferencesWindow
