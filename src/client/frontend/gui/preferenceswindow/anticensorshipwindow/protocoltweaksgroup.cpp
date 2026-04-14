#include "protocoltweaksgroup.h"

#include <QPainter>
#include <QSettings>

#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

ProtocolTweaksGroup::ProtocolTweaksGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    checkBoxEnable_ = new ToggleItem(this);
    checkBoxEnable_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/CIRCUMVENT_CENSORSHIP"));
    connect(checkBoxEnable_, &ToggleItem::stateChanged, this, &ProtocolTweaksGroup::onCheckBoxStateChanged);
    addItem(checkBoxEnable_);

    amneziawgPreset_ = new ComboBoxItem(this);
    amneziawgPreset_->setCaptionFont(FontDescr(14, QFont::Normal));
    connect(amneziawgPreset_, &ComboBoxItem::currentItemChanged, this, &ProtocolTweaksGroup::onAmneziawgPresetChanged);
    addItem(amneziawgPreset_);

    hideItems(indexOf(amneziawgPreset_), indexOf(amneziawgPreset_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ProtocolTweaksGroup::onLanguageChanged);
    onLanguageChanged();
}

void ProtocolTweaksGroup::setAntiCensorshipEnabled(bool enabled)
{
    if (enabled != isEnabled_) {
        isEnabled_ = enabled;
        checkBoxEnable_->setState(enabled);
        updateMode();
    }
}

void ProtocolTweaksGroup::setAmneziawgPreset(const QString &preset)
{
    if (amneziawgPreset_->hasItems()) {
        amneziawgPreset_->setCurrentItem(preset);
    }
}

void ProtocolTweaksGroup::setAmneziawgUnblockParams(const QString &activePreset, QStringList presets)
{
    amneziawgPreset_->clear();

    if (presets.isEmpty()) {
        // TODO: JDRM do we need some way to warn the user that we have no presets, indicating we are unable to retrieve them from the API?.
        return;
    }

    for (const auto &preset : presets) {
        amneziawgPreset_->addItem(preset, preset);
    }

    if (activePreset.isEmpty() || !presets.contains(activePreset)) {
        amneziawgPreset_->setCurrentItem(0);
    } else {
        amneziawgPreset_->setCurrentItem(activePreset);
    }
}

void ProtocolTweaksGroup::onCheckBoxStateChanged(bool isChecked)
{
    isEnabled_ = isChecked;
    updateMode();
    setAmneziaIsUserConfigured();
    emit antiCensorshipStateChanged(isEnabled_);
}

void ProtocolTweaksGroup::onAmneziawgPresetChanged(const QVariant &value)
{
    setAmneziaIsUserConfigured();
    emit amneziawgPresetChanged(value.toString());
}

void ProtocolTweaksGroup::updateMode()
{
    if (isEnabled_) {
        showItems(indexOf(amneziawgPreset_), size() - 1);
    }
    else {
        hideItems(indexOf(amneziawgPreset_), size() - 1);
    }
}

void ProtocolTweaksGroup::setAmneziaIsUserConfigured()
{
    QSettings settings;
    settings.setValue("AmneziaIsUserConfigured", true);
}

void ProtocolTweaksGroup::onLanguageChanged()
{
    checkBoxEnable_->setCaption(tr("Protocol Tweaks"));
    amneziawgPreset_->setLabelCaption(tr("Amnezia Config"));
    updateMode();
}

void ProtocolTweaksGroup::setEnabled(bool enabled)
{
    checkBoxEnable_->setEnabled(enabled);
    if (!enabled) {
        checkBoxEnable_->setState(false);
    }
}

void ProtocolTweaksGroup::setDescription(const QString &desc, const QString &descUrl)
{
    checkBoxEnable_->setDescription(desc, descUrl);
}

} // namespace PreferencesWindow
