#include "firewallgroup.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "languagecontroller.h"
#include "tooltips/tooltiputil.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

FirewallGroup::FirewallGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    firewallModeItem_ = new ComboBoxItem(this);
    connect(firewallModeItem_, &ComboBoxItem::currentItemChanged, this, &FirewallGroup::onFirewallModeChanged);
    firewallModeItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/FIREWALL_MODE"));
    addItem(firewallModeItem_);

    firewallWhenItem_ = new ComboBoxItem(this);
    connect(firewallWhenItem_, &ComboBoxItem::currentItemChanged, this, &FirewallGroup::onFirewallWhenChanged);
    firewallWhenItem_->setCaptionFont(FontDescr(14, QFont::Normal));
    addItem(firewallWhenItem_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &FirewallGroup::onLanguageChanged);
    onLanguageChanged();
}

void FirewallGroup::setFirewallSettings(types::FirewallSettings settings)
{
    settings_ = settings;
    firewallModeItem_->setCurrentItem(settings_.mode);
    firewallWhenItem_->setCurrentItem(settings_.when);
    updateMode();
}

void FirewallGroup::onFirewallModeChanged(QVariant value)
{
    if (settings_.mode != (FIREWALL_MODE)value.toInt())
    {
        settings_.mode = (FIREWALL_MODE)value.toInt();
        updateMode();
        emit firewallPreferencesChanged(settings_);
    }
}

void FirewallGroup::updateMode()
{
    if (settings_.mode == FIREWALL_MODE_AUTOMATIC)
    {
        showItems(indexOf(firewallWhenItem_));
    }
    else
    {
        hideItems(indexOf(firewallWhenItem_));
    }
}

void FirewallGroup::onFirewallWhenChanged(QVariant value)
{
    if (settings_.when != (FIREWALL_WHEN)value.toInt())
    {
        settings_.when = (FIREWALL_WHEN)value.toInt();
        emit firewallPreferencesChanged(settings_);
    }
}

void FirewallGroup::onLanguageChanged()
{
    firewallModeItem_->setLabelCaption(tr("Firewall Mode"));
    firewallModeItem_->setItems(FIREWALL_MODE_toList(), settings_.mode);
    firewallWhenItem_->setLabelCaption(tr("When?"));
    firewallWhenItem_->setItems(FIREWALL_WHEN_toList(), settings_.when);
}

void FirewallGroup::setDescription(const QString &desc, const QString &descUrl)
{
    firewallModeItem_->setDescription(desc, descUrl);
}

} // namespace PreferencesWindow