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

    comboBoxMode_ = new ComboBoxItem(this);
    comboBoxMode_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/CIRCUMVENT_CENSORSHIP"));
    connect(comboBoxMode_, &ComboBoxItem::currentItemChanged, this, &ProtocolTweaksGroup::onComboBoxModeChanged);
    addItem(comboBoxMode_);

    amneziawgPreset_ = new ComboBoxItem(this);
    amneziawgPreset_->setCaptionFont(FontDescr(14, QFont::Normal));
    connect(amneziawgPreset_, &ComboBoxItem::currentItemChanged, this, &ProtocolTweaksGroup::onAmneziawgPresetChanged);
    addItem(amneziawgPreset_);

    customSniDomainEdit_ = new EditBoxItem(this, tr("Custom SNI Domain"), "example.com");
    connect(customSniDomainEdit_, &EditBoxItem::textChanged, this, &ProtocolTweaksGroup::onCustomSniDomainChanged);
    addItem(customSniDomainEdit_);

    hideItems(indexOf(amneziawgPreset_), indexOf(customSniDomainEdit_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ProtocolTweaksGroup::onLanguageChanged);
    onLanguageChanged();
}

void ProtocolTweaksGroup::setProtocolTweaksMethod(PROTOCOL_TWEAKS_METHOD_TYPE method)
{
    if (method != method_) {
        method_ = method;
        comboBoxMode_->setCurrentItem(static_cast<int>(method));
        updateMode();
    }
}

void ProtocolTweaksGroup::setAmneziawgPreset(const QString &preset)
{
    if (amneziawgPreset_->hasItems()) {
        amneziawgPreset_->setCurrentItem(preset);
    }
}

void ProtocolTweaksGroup::setAmneziawgPresets(const QStringList &presets)
{
    amneziawgPreset_->clear();

    // TODO: JDRM do we need some way to warn the user that we have no presets, indicating we are unable to retrieve them from the API?.
    for (const auto &preset : presets) {
        amneziawgPreset_->addItem(preset, preset);
    }

    // Refresh visibility: an empty combobox must stay hidden even in Enabled mode
    // (otherwise opening it would assert on an uninitialized current item).
    updateMode();
}

void ProtocolTweaksGroup::setCustomSniDomain(const QString &domain)
{
    customSniDomainEdit_->setText(domain);
}

QString ProtocolTweaksGroup::customSniDomain() const
{
    return customSniDomainEdit_->text();
}

void ProtocolTweaksGroup::onComboBoxModeChanged(const QVariant &value)
{
    method_ = enumFromInt<PROTOCOL_TWEAKS_METHOD_TYPE>(value.toInt());
    updateMode();
    emit protocolTweaksMethodChanged(method_);
}

void ProtocolTweaksGroup::onAmneziawgPresetChanged(const QVariant &value)
{
    emit amneziawgPresetChanged(value.toString());
}

void ProtocolTweaksGroup::onCustomSniDomainChanged(const QString &domain)
{
    emit customSniDomainChanged(domain);
}

void ProtocolTweaksGroup::updateMode()
{
    if (method_ == PROTOCOL_TWEAKS_METHOD_ENABLED) {
        // An empty preset combobox must stay hidden even in Enabled mode, but the custom SNI
        // field is an independent preference and must be reachable regardless of preset availability.
        if (amneziawgPreset_->hasItems()) {
            showItems(indexOf(amneziawgPreset_));
        } else {
            hideItems(indexOf(amneziawgPreset_));
        }
        showItems(indexOf(customSniDomainEdit_));
    } else {
        hideItems(indexOf(amneziawgPreset_), size() - 1);
    }
}

void ProtocolTweaksGroup::onLanguageChanged()
{
    comboBoxMode_->setLabelCaption(tr("Protocol Tweaks"));
    comboBoxMode_->setItems(enumToList<PROTOCOL_TWEAKS_METHOD_TYPE>(), method_);
    amneziawgPreset_->setLabelCaption(tr("Amnezia Config"));
    customSniDomainEdit_->setCaption(tr("Custom SNI Domain"));
    customSniDomainEdit_->setPrompt("example.com");
    updateMode();
}

void ProtocolTweaksGroup::setDescription(const QString &desc, const QString &descUrl)
{
    comboBoxMode_->setDescription(desc, descUrl);
}

} // namespace PreferencesWindow
