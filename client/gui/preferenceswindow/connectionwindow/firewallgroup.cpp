#include "firewallgroup.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "tooltips/tooltiputil.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

FirewallGroup::FirewallGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    firewallModeItem_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Firewall Mode"), QString());
    const QList< QPair<QString, int> > modes = FIREWALL_MODE_toList();
    for (const auto v : modes)
    {
        firewallModeItem_->addItem(v.first, v.second);
    }
    connect(firewallModeItem_, &ComboBoxItem::currentItemChanged, this, &FirewallGroup::onFirewallModeChanged);
    firewallModeItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/FIREWALL_MODE"));
    addItem(firewallModeItem_);

    firewallWhenItem_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "When?"), QString());
    const QList< QPair<QString, int> > whens = FIREWALL_WHEN_toList();
    for (const auto v : whens)
    {
        firewallWhenItem_->addItem(v.first, v.second);
    }
    connect(firewallWhenItem_, &ComboBoxItem::currentItemChanged, this, &FirewallGroup::onFirewallWhenChanged);
    firewallWhenItem_->setCaptionFont(FontDescr(12, false));
    addItem(firewallWhenItem_);
}

void FirewallGroup::setFirewallSettings(types::FirewallSettings settings)
{
    settings_ = settings;
    firewallModeItem_->setCurrentItem(settings_.mode);
    firewallWhenItem_->setCurrentItem(settings_.when);
    updateMode();
}

void FirewallGroup::setBlock(bool block)
{
    block_ = block;
    firewallModeItem_->setClickable(!block);
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

void FirewallGroup::onFirewallModeHoverEnter()
{
    if (!block_)
        return;

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(firewallModeItem_->getButtonScenePos()));

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_FIREWALL_BLOCKED);
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    ti.x = globalPt.x() + 8 * G_SCALE;
    ti.y = globalPt.y() - 4 * G_SCALE;
    ti.width = 200 * G_SCALE;
    TooltipUtil::getFirewallBlockedTooltipInfo(&ti.title, &ti.desc);
    TooltipController::instance().showTooltipDescriptive(ti);
}

void FirewallGroup::onFirewallModeHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_FIREWALL_BLOCKED);
}

} // namespace PreferencesWindow