#include "splittunnelingappswindowitem.h"

#include "languagecontroller.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

SplitTunnelingAppsWindowItem::SplitTunnelingAppsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences), loggedIn_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    desc_ = new PreferenceGroup(this);
    desc_->setDescriptionBorderWidth(2);
    addItem(desc_);

    splitTunnelingAppsGroup_ = new SplitTunnelingAppsGroup(this);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::appsUpdated, this, &SplitTunnelingAppsWindowItem::onAppsUpdated);
    connect(splitTunnelingAppsGroup_, &SplitTunnelingAppsGroup::setError, this, &SplitTunnelingAppsWindowItem::onError);
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
    // Clears error and sets the default description
    setLoggedIn(loggedIn_);

    preferences_->setSplitTunnelingApps(apps);
    emit appsUpdated(apps);
}

void SplitTunnelingAppsWindowItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;

    if (loggedIn) {
        desc_->clearError();
        QString desc = tr("Add the apps you wish to include in or exclude from the VPN tunnel below.");
        desc_->setDescription(desc, false);
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

void SplitTunnelingAppsWindowItem::onError(QString msg)
{
    desc_->setDescription(msg, true);
}

} // namespace PreferencesWindow
