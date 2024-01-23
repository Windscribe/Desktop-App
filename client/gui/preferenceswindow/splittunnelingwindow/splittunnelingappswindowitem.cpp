#include "splittunnelingappswindowitem.h"

#include "languagecontroller.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

SplitTunnelingAppsWindowItem::SplitTunnelingAppsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences), loggedIn_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    desc_ = new PreferenceGroup(this);
    desc_->setDescriptionBorderWidth(2);
    addItem(desc_);

    splitTunnelingAppsGroup_ = new SplitTunnelingAppsGroup(this);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::appsUpdated, this, &SplitTunnelingAppsWindowItem::onAppsUpdated);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::addClicked, this, &SplitTunnelingAppsWindowItem::addButtonClicked);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::escape, this, &SplitTunnelingAppsWindowItem::escape);
    addItem(splitTunnelingAppsGroup_);
    setFocusProxy(splitTunnelingAppsGroup_);

    connect(preferences_, &Preferences::splitTunnelingChanged, this, &SplitTunnelingAppsWindowItem::onPreferencesChanged);
    onPreferencesChanged();

    setLoggedIn(false);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SplitTunnelingAppsWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString SplitTunnelingAppsWindowItem::caption() const
{
    return tr("Apps");
}

QList<types::SplitTunnelingApp> SplitTunnelingAppsWindowItem::getApps()
{
    return splitTunnelingAppsGroup_->apps();
}

void SplitTunnelingAppsWindowItem::addAppManually(types::SplitTunnelingApp app)
{
    splitTunnelingAppsGroup_->addApp(app);
}

void SplitTunnelingAppsWindowItem::onAppsUpdated(QList<types::SplitTunnelingApp> apps)
{
    preferences_->setSplitTunnelingApps(apps);
    emit appsUpdated(apps);
}

void SplitTunnelingAppsWindowItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;

    if (loggedIn) {
        desc_->clearError();
        desc_->setDescription(tr("Add the apps you wish to include in or exclude from the VPN tunnel below."));
    } else {
        desc_->setDescription(tr("Please log in to modify split tunneling rules."), true);
    }

    splitTunnelingAppsGroup_->setLoggedIn(loggedIn);
}

void SplitTunnelingAppsWindowItem::onLanguageChanged()
{
    setLoggedIn(loggedIn_);
}

void SplitTunnelingAppsWindowItem::onPreferencesChanged()
{
    splitTunnelingAppsGroup_->setApps(preferences_->splitTunnelingApps());
}

} // namespace PreferencesWindow
