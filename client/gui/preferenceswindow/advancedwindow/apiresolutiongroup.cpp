#include "apiresolutiongroup.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ApiResolutionGroup::ApiResolutionGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    resolutionModeItem_ = new ComboBoxItem(this, tr("API Resolution"), QString());
    resolutionModeItem_->addItem(tr("Automatic"), 0);
    resolutionModeItem_->addItem(tr("Manual"), 1);
    resolutionModeItem_->setCurrentItem(0);
    resolutionModeItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/API_RESOLUTION"));
    connect(resolutionModeItem_, &ComboBoxItem::currentItemChanged, this, &ApiResolutionGroup::onAutomaticChanged);
    addItem(resolutionModeItem_);

    editBoxAddress_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter IP or Hostname"));
    connect(editBoxAddress_, &EditBoxItem::textChanged, this, &ApiResolutionGroup::onAddressChanged);
    addItem(editBoxAddress_);
    hideItems(indexOf(editBoxAddress_));
}

void ApiResolutionGroup::setApiResolution(const types::ApiResolutionSettings &ar)
{
    if(settings_ != ar)
    {
        settings_ = ar;
        resolutionModeItem_->setCurrentItem(settings_.getIsAutomatic() ? 0 : 1);
        editBoxAddress_->setText(settings_.getManualAddress());
        updateMode();
    }
}

bool ApiResolutionGroup::hasItemWithFocus()
{
    return editBoxAddress_->lineEditHasFocus();
}

void ApiResolutionGroup::onAutomaticChanged(QVariant value)
{
    settings_.setIsAutomatic(value.toInt() == 0);
    updateMode();
    emit apiResolutionChanged(settings_);
}

void ApiResolutionGroup::onAddressChanged(const QString &text)
{
    settings_.setManualAddress(text.trimmed());
    emit apiResolutionChanged(settings_);
}

void ApiResolutionGroup::updateMode()
{
    if (settings_.getIsAutomatic())
    {
        hideItems(indexOf(editBoxAddress_));
    }
    else
    {
        showItems(indexOf(editBoxAddress_));
    }
}

} // namespace PreferencesWindow
