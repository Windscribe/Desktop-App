#include "splittunnelinggroup.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "tooltips/tooltiputil.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/logger.h"
#include "dpiscalemanager.h"

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
#endif

namespace PreferencesWindow {

SplitTunnelingGroup::SplitTunnelingGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    activeCheckBox_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Split Tunneling"), QString());
    activeCheckBox_->setState(true);
    activeCheckBox_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SPLIT_TUNNELING"));
    connect(activeCheckBox_, &CheckBoxItem::stateChanged, this, &SplitTunnelingGroup::onActiveSwitchChanged);
    addItem(activeCheckBox_);

    modeComboBox_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Mode"), "");
    modeComboBox_->setCaptionFont(FontDescr(12, false));
    modeComboBox_->addItem(tr("Exclusive"), SPLIT_TUNNELING_MODE_EXCLUDE);
    modeComboBox_->addItem(tr("Inclusive"), SPLIT_TUNNELING_MODE_INCLUDE);
    connect(modeComboBox_, &ComboBoxItem::currentItemChanged, this, &SplitTunnelingGroup::onCurrentModeChanged);
    addItem(modeComboBox_);

    appsLinkItem_ = new LinkItem(this, LinkItem::LinkType::SUBPAGE_LINK, QT_TRANSLATE_NOOP("PreferencesWindow::LinkItem", tr("Apps")));
    connect(appsLinkItem_, &LinkItem::clicked, this, &SplitTunnelingGroup::appsPageClick);
    addItem(appsLinkItem_);

    addressesLinkItem_ = new LinkItem(this, LinkItem::LinkType::SUBPAGE_LINK, QT_TRANSLATE_NOOP("PreferencesWindow::LinkItem", tr("IPs & Hostnames")));
    connect(addressesLinkItem_, &LinkItem::clicked, this, &SplitTunnelingGroup::addressesPageClick);
    addItem(addressesLinkItem_);

    updateScaling();
}

void SplitTunnelingGroup::setSettings(types::SplitTunnelingSettings settings)
{
#ifdef Q_OS_MAC
    if (settings.active && MacUtils::isOsVersionIsBigSur_or_greater()) {
        settings.active = false;
        emit settingsChanged(settings);
    }
#endif
    settings_ = settings;
    activeCheckBox_->setState(settings.active);
    onActiveSwitchChanged(settings.active);
    if (modeComboBox_->hasItems())
    {
        modeComboBox_->setCurrentItem(settings.mode);
    }
    onCurrentModeChanged(settings.mode);
}

void SplitTunnelingGroup::onActiveSwitchChanged(bool checked)
{
    if (settings_.active != checked) {
        settings_.active = checked;
        emit settingsChanged(settings_);
    }

    if (checked)
    {
        showDescription();
        showItems(indexOf(modeComboBox_), size() - 1);
    }
    else
    {
        hideDescription();
        hideItems(indexOf(modeComboBox_), size() - 1);
    }
}

void SplitTunnelingGroup::onCurrentModeChanged(QVariant value)
{
    SPLIT_TUNNELING_MODE mode = (SPLIT_TUNNELING_MODE)value.toInt();
    settings_.mode = mode;
    emit settingsChanged(settings_);

    updateDescription();
}

void SplitTunnelingGroup::setAppsCount(int count)
{
    QString num(QString("%1").arg(count));
    appsLinkItem_->setLinkText(num);
}

void SplitTunnelingGroup::setAddressesCount(int count)
{
    QString num(QString("%1").arg(count));
    addressesLinkItem_->setLinkText(num);
}

void SplitTunnelingGroup::updateDescription()
{
    switch(settings_.mode)
    {
        case SPLIT_TUNNELING_MODE_EXCLUDE:
            setDescription(tr("Selected apps, IPs, and hostnames will not go through Windscribe when connected."));
            break;
        case SPLIT_TUNNELING_MODE_INCLUDE:
            setDescription(tr("Only selected apps, IPs, and hostnames will go through Windscribe when connected."));
            break;
        default:
            break;
    }
}

void SplitTunnelingGroup::setEnabled(bool enabled)
{
    activeCheckBox_->setEnabled(enabled);
}

}