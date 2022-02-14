#include "splittunnelingappssearchwindowitem.h"

namespace PreferencesWindow {

SplitTunnelingAppsSearchWindowItem::SplitTunnelingAppsSearchWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage (parent)
    , preferences_(preferences)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    splitTunnelingAppsSearchItem_ = new SplitTunnelingAppsSearchItem(this);
    connect(splitTunnelingAppsSearchItem_, SIGNAL(appsUpdated(QList<ProtoTypes::SplitTunnelingApp>)), SLOT(onAppsUpdated(QList<ProtoTypes::SplitTunnelingApp>)));
    connect(splitTunnelingAppsSearchItem_, SIGNAL(searchModeExited()), SIGNAL(searchModeExited()));
    connect(splitTunnelingAppsSearchItem_, SIGNAL(nativeInfoErrorMessage(QString,QString)), SIGNAL(nativeInfoErrorMessage(QString,QString)));
    connect(splitTunnelingAppsSearchItem_, SIGNAL(scrollToRect(QRect)), SIGNAL(scrollToRect(QRect)));
    addItem(splitTunnelingAppsSearchItem_);
    // setFocusProxy(splitTunnelingAppsSearchItem_);
    splitTunnelingAppsSearchItem_->setApps(preferences->splitTunnelingApps());
}

QString SplitTunnelingAppsSearchWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Apps Search");
}

void SplitTunnelingAppsSearchWindowItem::setFocusOnSearchBar()
{
    splitTunnelingAppsSearchItem_->setFocusOnSearchBar();
}

QList<ProtoTypes::SplitTunnelingApp> SplitTunnelingAppsSearchWindowItem::getApps()
{
    return splitTunnelingAppsSearchItem_->getApps();
}

void SplitTunnelingAppsSearchWindowItem::setApps(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    splitTunnelingAppsSearchItem_->setApps(apps);
}

void SplitTunnelingAppsSearchWindowItem::setLoggedIn(bool loggedIn)
{
    splitTunnelingAppsSearchItem_->setLoggedIn(loggedIn);
}

void SplitTunnelingAppsSearchWindowItem::updateProgramList()
{
    splitTunnelingAppsSearchItem_->updateProgramList();
}

void SplitTunnelingAppsSearchWindowItem::onAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    preferences_->setSplitTunnelingApps(apps);
    emit appsUpdated(apps); // update other screen info
}

} // namespace
