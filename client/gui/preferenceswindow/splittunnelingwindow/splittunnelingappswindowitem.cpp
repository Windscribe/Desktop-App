#include "splittunnelingappswindowitem.h"

namespace PreferencesWindow {

PreferencesWindow::SplitTunnelingAppsWindowItem::SplitTunnelingAppsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage (parent)
  , preferences_(preferences)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    splitTunnelingAppsItem_ = new SplitTunnelingAppsItem(this);
    connect(splitTunnelingAppsItem_, SIGNAL(appsUpdated(QList<ProtoTypes::SplitTunnelingApp>)), SLOT(onAppsUpdated(QList<ProtoTypes::SplitTunnelingApp>)));
    connect(splitTunnelingAppsItem_, SIGNAL(searchClicked()), SIGNAL(searchButtonClicked()));
    connect(splitTunnelingAppsItem_, SIGNAL(addClicked()), SIGNAL(addButtonClicked()));
    connect(splitTunnelingAppsItem_, SIGNAL(nativeInfoErrorMessage(QString, QString)), SIGNAL(nativeInfoErrorMessage(QString,QString)));
    connect(splitTunnelingAppsItem_, SIGNAL(scrollToRect(QRect)), SIGNAL(scrollToRect(QRect)));
    connect(splitTunnelingAppsItem_, SIGNAL(escape()), SIGNAL(escape()));
    addItem(splitTunnelingAppsItem_);
    setFocusProxy(splitTunnelingAppsItem_);

    splitTunnelingAppsItem_->setApps(preferences->splitTunnelingApps());
}

QString SplitTunnelingAppsWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Apps");
}

QList<ProtoTypes::SplitTunnelingApp> SplitTunnelingAppsWindowItem::getApps()
{
    return splitTunnelingAppsItem_->getApps();
}

void SplitTunnelingAppsWindowItem::setApps(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    splitTunnelingAppsItem_->setApps(apps);
}

void SplitTunnelingAppsWindowItem::addAppManually(ProtoTypes::SplitTunnelingApp app)
{
    QList<ProtoTypes::SplitTunnelingApp> apps = splitTunnelingAppsItem_->getApps();
    apps.append(app);
    splitTunnelingAppsItem_->setApps(apps);
    preferences_->setSplitTunnelingApps(apps);
    emit appsUpdated(apps);
}

void SplitTunnelingAppsWindowItem::setLoggedIn(bool able)
{
    splitTunnelingAppsItem_->setLoggedIn(able);
}

void SplitTunnelingAppsWindowItem::onAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    preferences_->setSplitTunnelingApps(apps);
    emit appsUpdated(apps);
}

} // namespace
