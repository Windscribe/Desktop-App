#include "anticensorshipwindowitem.h"

#include <QPainter>

#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencegroup.h"
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

AntiCensorshipWindowItem::AntiCensorshipWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences)
{
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    connect(preferences, &Preferences::isAntiCensorshipChanged, this, &AntiCensorshipWindowItem::onAntiCensorshipPreferencesChanged);
    connect(preferences, &Preferences::amneziawgPresetChanged, this, &AntiCensorshipWindowItem::onAmneziawgPresetChanged);
    connect(preferences, &Preferences::serverRoutingMethodChanged, this, &AntiCensorshipWindowItem::onServerRoutingMethodPreferencesChanged);


    desc_ = new PreferenceGroup(this);
    addItem(desc_);

    protocolTweaksGroup_ = new ProtocolTweaksGroup(this);
    connect(protocolTweaksGroup_, &ProtocolTweaksGroup::antiCensorshipStateChanged, this, &AntiCensorshipWindowItem::onAntiCensorshipPreferencesChangedByUser);
    connect(protocolTweaksGroup_, &ProtocolTweaksGroup::amneziawgPresetChanged, this, &AntiCensorshipWindowItem::onAmneziawgPresetChangedByUser);
    protocolTweaksGroup_->setAntiCensorshipEnabled(preferences->isAntiCensorship());
    addItem(protocolTweaksGroup_);

    serverRoutingGroup_ = new PreferenceGroup(this);
    comboBoxServerRouting_ = new ComboBoxItem(serverRoutingGroup_);
    connect(comboBoxServerRouting_, &ComboBoxItem::currentItemChanged, this, &AntiCensorshipWindowItem::onServerRoutingMethodChangedByUser);
    serverRoutingGroup_->addItem(comboBoxServerRouting_);
    addItem(serverRoutingGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &AntiCensorshipWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString AntiCensorshipWindowItem::caption() const
{
    return tr("Anti-censorship");
}

void AntiCensorshipWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
}

void AntiCensorshipWindowItem::setAmneziawgUnblockParams(const QString &activePreset, QStringList presets)
{
    protocolTweaksGroup_->setAmneziawgUnblockParams(activePreset, presets);
}

void AntiCensorshipWindowItem::onLanguageChanged()
{
    desc_->setDescription(tr("These degrade performance, enable only if Windscribe doesn't connect normally."),
                          QString("https://%1/features/circumvent-censorship").arg(HardcodedSettings::instance().windscribeServerUrl()));

    protocolTweaksGroup_->setDescription(tr("Protocol-level changes made to WireGuard, OpenVPN, and Stealth protocols."));

    serverRoutingGroup_->setDescription(tr("Increases latency, but improves chances of being able to connect."));
    comboBoxServerRouting_->setLabelCaption(tr("Server Routing"));
    comboBoxServerRouting_->setItems(SERVER_ROUTING_METHOD_TYPE_toList(), preferences_->serverRoutingMethod());
}

void AntiCensorshipWindowItem::onAntiCensorshipPreferencesChanged(bool b)
{
    protocolTweaksGroup_->setAntiCensorshipEnabled(b);
}

void AntiCensorshipWindowItem::onAmneziawgPresetChanged(const QString &preset)
{
    protocolTweaksGroup_->setAmneziawgPreset(preset);
}

void AntiCensorshipWindowItem::onAntiCensorshipPreferencesChangedByUser(bool isChecked)
{
    preferences_->setAntiCensorship(isChecked);
}

void AntiCensorshipWindowItem::onAmneziawgPresetChangedByUser(const QString &preset)
{
    preferences_->setAmneziawgPreset(preset);
}

void AntiCensorshipWindowItem::onServerRoutingMethodPreferencesChanged(SERVER_ROUTING_METHOD_TYPE method)
{
    comboBoxServerRouting_->setCurrentItem(static_cast<int>(method));
}

void AntiCensorshipWindowItem::onServerRoutingMethodChangedByUser(QVariant method)
{
    preferences_->setServerRoutingMethod(static_cast<SERVER_ROUTING_METHOD_TYPE>(method.toInt()));
}

} // namespace PreferencesWindow
