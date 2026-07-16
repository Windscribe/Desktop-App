#include "anticensorshipwindowitem.h"

#include <QPainter>

#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencegroup.h"
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

AntiCensorshipWindowItem::AntiCensorshipWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
    : CommonGraphics::BasePage(parent), preferences_(preferences), preferencesHelper_(preferencesHelper)
{
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    connect(preferences, &Preferences::protocolTweaksMethodChanged, this, &AntiCensorshipWindowItem::onProtocolTweaksMethodPreferencesChanged);
    connect(preferences, &Preferences::amneziawgPresetChanged, this, &AntiCensorshipWindowItem::onAmneziawgPresetChanged);
    connect(preferences, &Preferences::customSniDomainChanged, this, &AntiCensorshipWindowItem::onCustomSniDomainPreferencesChanged);
    connect(preferences, &Preferences::serverRoutingMethodChanged, this, &AntiCensorshipWindowItem::onServerRoutingMethodPreferencesChanged);
    connect(preferences, &Preferences::isAPIAntiCensorshipChanged, this, &AntiCensorshipWindowItem::onIsAPIAntiCensorshipPreferencesChanged);
    connect(preferencesHelper, &PreferencesHelper::amneziawgPresetsChanged, this, &AntiCensorshipWindowItem::onAmneziawgPresetsChanged);
    connect(preferencesHelper, &PreferencesHelper::apiSuggestedCustomSniChanged, this, &AntiCensorshipWindowItem::onApiSuggestedCustomSniChanged);

    desc_ = new PreferenceGroup(this);
    addItem(desc_);

    protocolTweaksGroup_ = new ProtocolTweaksGroup(this);
    connect(protocolTweaksGroup_, &ProtocolTweaksGroup::protocolTweaksMethodChanged, this, &AntiCensorshipWindowItem::onProtocolTweaksMethodChangedByUser);
    connect(protocolTweaksGroup_, &ProtocolTweaksGroup::amneziawgPresetChanged, this, &AntiCensorshipWindowItem::onAmneziawgPresetChangedByUser);
    protocolTweaksGroup_->setProtocolTweaksMethod(preferences->protocolTweaksMethod());
    applyAmneziawgPresets(preferencesHelper->amneziawgPresets());
    addItem(protocolTweaksGroup_);

    serverRoutingGroup_ = new PreferenceGroup(this);
    comboBoxServerRouting_ = new ComboBoxItem(serverRoutingGroup_);
    connect(comboBoxServerRouting_, &ComboBoxItem::currentItemChanged, this, &AntiCensorshipWindowItem::onServerRoutingMethodChangedByUser);
    serverRoutingGroup_->addItem(comboBoxServerRouting_);
    addItem(serverRoutingGroup_);

    largeTlsGroup_ = new PreferenceGroup(this);
    toggleLargeTls_ = new ToggleItem(largeTlsGroup_);
    toggleLargeTls_->setState(preferences->isAPIAntiCensorship());
    connect(toggleLargeTls_, &ToggleItem::stateChanged, this, &AntiCensorshipWindowItem::onLargeTlsToggleChangedByUser);
    largeTlsGroup_->addItem(toggleLargeTls_);
    addItem(largeTlsGroup_);

    customSniGroup_ = new PreferenceGroup(this);
    customSniDomainEdit_ = new VerticalEditBoxItem(customSniGroup_);
    customSniDomainEdit_->setText(preferences->customSniDomain());
    connect(customSniDomainEdit_, &VerticalEditBoxItem::textChanged, this, &AntiCensorshipWindowItem::onCustomSniDomainChangedByUser);
    customSniGroup_->addItem(customSniDomainEdit_);
    addItem(customSniGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &AntiCensorshipWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString AntiCensorshipWindowItem::caption() const
{
    return tr("Anti-Censorship");
}

void AntiCensorshipWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
}

void AntiCensorshipWindowItem::onLanguageChanged()
{
    desc_->setDescription(tr("These degrade performance, enable only if Windscribe doesn't connect normally."),
                          QString("https://%1/features/circumvent-censorship").arg(HardcodedSettings::instance().windscribeServerUrl()));

    protocolTweaksGroup_->setDescription(tr("Protocol-level changes made to WireGuard, OpenVPN, and Stealth protocols."));

    serverRoutingGroup_->setDescription(tr("Increases latency, but improves chances of being able to connect."));
    comboBoxServerRouting_->setLabelCaption(tr("Server Routing"));
    comboBoxServerRouting_->setItems(enumToList<SERVER_ROUTING_METHOD_TYPE>(), preferences_->serverRoutingMethod());

    toggleLargeTls_->setCaption(tr("Large TLS"));
    largeTlsGroup_->setDescription(tr("Artificially enlarge TLS packets, helps to circumvent censorship in some cases. Adds extra TLS padding to all API requests."));

    customSniDomainEdit_->setCaption(tr("Custom SNI Domain"));
    customSniDomainEdit_->setPrompt("example.com");
    customSniGroup_->setDescription(tr("This setting applies to Stealth and Wstunnel protocols only."));
}

void AntiCensorshipWindowItem::onProtocolTweaksMethodPreferencesChanged(PROTOCOL_TWEAKS_METHOD_TYPE method)
{
    protocolTweaksGroup_->setProtocolTweaksMethod(method);
}

void AntiCensorshipWindowItem::onAmneziawgPresetChanged(const QString &preset)
{
    protocolTweaksGroup_->setAmneziawgPreset(preset);
}

void AntiCensorshipWindowItem::onCustomSniDomainPreferencesChanged(const QString &domain)
{
    customSniDomainEdit_->setText(domain);
}

void AntiCensorshipWindowItem::onApiSuggestedCustomSniChanged(const QString &domain)
{
    // API suggestion is informational only - user can see it but we don't auto-apply it
    // The user's configured value (if any) takes precedence
    Q_UNUSED(domain);
}

void AntiCensorshipWindowItem::onAmneziawgPresetsChanged(const QStringList &presets)
{
    applyAmneziawgPresets(presets);
}

void AntiCensorshipWindowItem::applyAmneziawgPresets(const QStringList &presets)
{
    protocolTweaksGroup_->setAmneziawgPresets(presets);

    const QString saved = preferences_->amneziawgPreset();
    if (!presets.isEmpty() && !presets.contains(saved)) {
        // Saved preset is no longer offered by the API. Fall back to the first one
        // and persist the change so the engine and UI stay in sync (otherwise the
        // combobox would silently show the first item while preferences still hold
        // the stale value).
        preferences_->setAmneziawgPreset(presets.first());
    } else {
        protocolTweaksGroup_->setAmneziawgPreset(saved);
    }
}

void AntiCensorshipWindowItem::onProtocolTweaksMethodChangedByUser(PROTOCOL_TWEAKS_METHOD_TYPE method)
{
    preferences_->setProtocolTweaksMethod(method);
}

void AntiCensorshipWindowItem::onAmneziawgPresetChangedByUser(const QString &preset)
{
    preferences_->setAmneziawgPreset(preset);
}

void AntiCensorshipWindowItem::onCustomSniDomainChangedByUser(const QString &domain)
{
    preferences_->setCustomSniDomain(domain);
}

void AntiCensorshipWindowItem::onServerRoutingMethodPreferencesChanged(SERVER_ROUTING_METHOD_TYPE method)
{
    comboBoxServerRouting_->setCurrentItem(static_cast<int>(method));
}

void AntiCensorshipWindowItem::onServerRoutingMethodChangedByUser(QVariant method)
{
    preferences_->setServerRoutingMethod(static_cast<SERVER_ROUTING_METHOD_TYPE>(method.toInt()));
}

void AntiCensorshipWindowItem::onIsAPIAntiCensorshipPreferencesChanged(bool b)
{
    if (toggleLargeTls_->isChecked() != b) {
        toggleLargeTls_->setState(b);
    }
}

void AntiCensorshipWindowItem::onLargeTlsToggleChangedByUser(bool isChecked)
{
    preferences_->setAPIAntiCensorship(isChecked);
}

} // namespace PreferencesWindow
