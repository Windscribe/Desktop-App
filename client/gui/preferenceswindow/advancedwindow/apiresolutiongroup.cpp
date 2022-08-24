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

    editBoxIP_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "IP Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter IP"));
    connect(editBoxIP_, &EditBoxItem::textChanged, this, &ApiResolutionGroup::onIPChanged);
    addItem(editBoxIP_);
    hideItems(indexOf(editBoxIP_));

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegularExpression ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    editBoxIP_->setValidator(ipValidator);
}

void ApiResolutionGroup::setApiResolution(const types::DnsResolutionSettings &ar)
{
    if(settings_ != ar)
    {
        settings_ = ar;
        resolutionModeItem_->setCurrentItem(settings_.getIsAutomatic() ? 0 : 1);
        editBoxIP_->setText(settings_.getManualIp());
        updateMode();
    }
}

bool ApiResolutionGroup::hasItemWithFocus()
{
    return editBoxIP_->lineEditHasFocus();
}

void ApiResolutionGroup::onAutomaticChanged(QVariant value)
{
    settings_.setIsAutomatic(value.toInt() == 0);
    updateMode();
    emit apiResolutionChanged(settings_);
}

void ApiResolutionGroup::onIPChanged(const QString &text)
{
    settings_.setManualIp(text.trimmed());
    emit apiResolutionChanged(settings_);
}

void ApiResolutionGroup::updateMode()
{
    if (settings_.getIsAutomatic())
    {
        hideItems(indexOf(editBoxIP_));
    }
    else
    {
        showItems(indexOf(editBoxIP_));
    }
}

} // namespace PreferencesWindow
