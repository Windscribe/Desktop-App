#include "mixedcomboboxitem.h"

#include <QFile>
#include <QPainter>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"

namespace PreferencesWindow {

MixedComboBoxItem::MixedComboBoxItem(ScalableGraphicsObject *parent, SoundManager *soundManager)
    : ComboBoxItem(parent), selectFileItem_(nullptr), secondaryComboBox_(nullptr), previewButton_(nullptr), soundManager_(soundManager)
{
    selectFileItem_ = new SelectFileItem(this);
    connect(selectFileItem_, &SelectFileItem::pathChanged, this, &MixedComboBoxItem::onPathChanged);

    secondaryComboBox_ = new ComboBoxItem(this);
    secondaryComboBox_->setCaptionY(0);
    secondaryComboBox_->setUseMinimumSize(true);
    secondaryComboBox_->setButtonFont(FontDescr(12, QFont::Normal));
    secondaryComboBox_->setButtonIcon("preferences/CNTXT_MENU_SMALL_ICON");
    connect(secondaryComboBox_, &ComboBoxItem::currentItemChanged, this, &MixedComboBoxItem::pathChanged);
    connect(secondaryComboBox_, &ComboBoxItem::currentItemChanged, this, &MixedComboBoxItem::updatePositions);

    previewButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/SOUND_PREVIEW_ICON", "", this, OPACITY_SEVENTY, OPACITY_FULL);
    connect(previewButton_, &IconButton::clicked, this, &MixedComboBoxItem::onPreviewButtonClicked);
    previewButton_->hide();

    if (soundManager_) {
        connect(soundManager_, &SoundManager::previewFinished, this, &MixedComboBoxItem::onPreviewPlaybackFinished);
    }

    connect(this, &ComboBoxItem::currentItemChanged, this, &MixedComboBoxItem::onCurrentItemChanged);

    selectFileItem_->hide();
    secondaryComboBox_->hide();

    updatePositions();
}

MixedComboBoxItem::~MixedComboBoxItem()
{
}

void MixedComboBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    ComboBoxItem::paint(painter, option, widget);
}

void MixedComboBoxItem::updateScaling()
{
    ComboBoxItem::updateScaling();
    updateSecondaryItemVisibility(false);
}

void MixedComboBoxItem::setCurrentItem(QVariant value)
{
    ComboBoxItem::setCurrentItem(value);
    updateSecondaryItemVisibility(false);
}

void MixedComboBoxItem::onCurrentItemChanged(QVariant value)
{
    Q_UNUSED(value);
    updateSecondaryItemVisibility();
}

void MixedComboBoxItem::onPathChanged(const QString &path)
{
    updatePreviewButtonVisibility();
    emit pathChanged(path);
}

void MixedComboBoxItem::updatePositions()
{
    int xIndent = isPreviewEnabled_ ? (ICON_WIDTH + 8)*G_SCALE : 0;
    int maxWidth = boundingRect().width() - 2*PREFERENCES_MARGIN_X*G_SCALE - buttonWidth() - 16*G_SCALE - xIndent;

    selectFileItem_->setPos(xIndent, kSecondaryItemMarginTop * G_SCALE);
    selectFileItem_->setMaxWidth(maxWidth);

    secondaryComboBox_->setPos(xIndent, kSecondaryItemMarginTop * G_SCALE);

    if (previewButton_) {
        int buttonY = (PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE - ICON_HEIGHT*G_SCALE) / 2;
        previewButton_->setPos(PREFERENCES_MARGIN_X*G_SCALE, buttonY);
    }
}

void MixedComboBoxItem::updateSecondaryItemVisibility(bool signal)
{
    if (!hasItems()) {
        return;
    }

    QVariant currentValue = currentItem();

    if (currentValue == customValue_) {
        selectFileItem_->show();
        secondaryComboBox_->hide();
        setCaptionY(8*G_SCALE, signal);
        if (signal) {
            emit pathChanged(selectFileItem_->path());
        }
    } else if (currentValue == bundledValue_) {
        selectFileItem_->hide();
        secondaryComboBox_->show();
        setCaptionY(8*G_SCALE, signal);
        if (signal) {
            emit pathChanged(secondaryComboBox_->currentItem());
        }
    } else {
        selectFileItem_->hide();
        secondaryComboBox_->hide();
        setCaptionY(-1, signal);
    }
    updatePreviewButtonVisibility();
}

void MixedComboBoxItem::setPath(const QString &path)
{
    if (currentItem() == customValue_) {
        selectFileItem_->setPath(path);
    } else if (currentItem() == bundledValue_) {
        secondaryComboBox_->setCurrentItem(path);
        updatePositions();
    }
}

void MixedComboBoxItem::setSecondaryItems(const QList<QPair<QString, QVariant>> &list, QVariant curValue)
{
    secondaryComboBox_->setItems(list, curValue);
}

void MixedComboBoxItem::setDialogText(const QString &title, const QString &filter)
{
    selectFileItem_->setDialogText(title, filter);
}

void MixedComboBoxItem::setEnablePreview(bool enable)
{
    isPreviewEnabled_ = enable;
    setCaptionXOffset(enable ? ICON_WIDTH + 8 : 0);
    updatePreviewButtonVisibility();
}

void MixedComboBoxItem::updatePreviewButtonVisibility()
{
    if (!previewButton_) {
        return;
    }

    if (!isPreviewEnabled_) {
        previewButton_->hide();
        return;
    }

    previewButton_->show();

    QVariant currentValue;
    bool isActive = false;
    if (hasItems()) {
        currentValue = currentItem();
        if (currentValue == bundledValue_) {
            isActive = true;
        } else if (currentValue == customValue_) {
            const QString path = selectFileItem_->path();
            isActive = !path.isEmpty() && QFile::exists(path);
        }
    }

    if (!isActive && isPlaying_) {
        if (soundManager_) {
            soundManager_->stop();
        }
        isPlaying_ = false;
        previewButton_->setIcon("preferences/SOUND_PREVIEW_ICON", false);
    }

    previewButton_->setClickableHoverable(isActive, isActive);
    previewButton_->setUnhoverOpacity(isActive ? OPACITY_SEVENTY : OPACITY_THIRD);
    previewButton_->setHoverOpacity(isActive ? OPACITY_FULL : OPACITY_THIRD);
    previewButton_->unhover();

    updatePositions();
}

QString MixedComboBoxItem::getCurrentSoundPath() const
{
    QVariant currentValue = currentItem();
    if (currentValue == customValue_) {
        return selectFileItem_->path();
    } else if (currentValue == bundledValue_) {
        return secondaryComboBox_->currentItem().toString();
    }
    return QString();
}

void MixedComboBoxItem::onPreviewButtonClicked()
{
    QString path = getCurrentSoundPath();
    if (path.isEmpty() || !soundManager_) {
        return;
    }

    if (isPlaying_) {
        soundManager_->stop();
        isPlaying_ = false;
        previewButton_->setIcon("preferences/SOUND_PREVIEW_ICON", false);
    } else {
        soundManager_->playPreview(path);
        isPlaying_ = true;
        previewButton_->setIcon("preferences/SOUND_STOP_ICON", false);
    }
}

void MixedComboBoxItem::onPreviewPlaybackFinished()
{
    isPlaying_ = false;
    if (previewButton_) {
        previewButton_->setIcon("preferences/SOUND_PREVIEW_ICON", false);
    }
}

} // namespace PreferencesWindow
