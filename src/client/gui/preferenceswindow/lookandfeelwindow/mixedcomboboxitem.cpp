#include "mixedcomboboxitem.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"

namespace PreferencesWindow {

MixedComboBoxItem::MixedComboBoxItem(ScalableGraphicsObject *parent)
    : ComboBoxItem(parent), selectFileItem_(nullptr), secondaryComboBox_(nullptr)
{
    selectFileItem_ = new SelectFileItem(this);
    connect(selectFileItem_, &SelectFileItem::pathChanged, this, &MixedComboBoxItem::onPathChanged);

    secondaryComboBox_ = new ComboBoxItem(this);
    secondaryComboBox_->setCaptionY(0);
    secondaryComboBox_->setUseMinimumSize(true);
    secondaryComboBox_->setButtonFont(FontDescr(12, QFont::Normal));
    secondaryComboBox_->setButtonIcon("preferences/CNTXT_MENU_SMALL_ICON");
    connect(secondaryComboBox_, &ComboBoxItem::currentItemChanged, this, &MixedComboBoxItem::pathChanged);

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
    updatePositions();
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
    updatePositions();
    emit pathChanged(path);
}

void MixedComboBoxItem::updatePositions()
{
    int maxWidth = boundingRect().width() - 2*PREFERENCES_MARGIN_X*G_SCALE - buttonWidth();

    selectFileItem_->setPos(0, kSecondaryItemMarginTop * G_SCALE);
    selectFileItem_->setMaxWidth(maxWidth);

    secondaryComboBox_->setPos(0, kSecondaryItemMarginTop * G_SCALE);
}

void MixedComboBoxItem::updateSecondaryItemVisibility(bool signal)
{
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
    updatePositions();
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

} // namespace PreferencesWindow
