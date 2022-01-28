#include "splittunnelingiphostnamewindowitem.h"

namespace PreferencesWindow {

SplitTunnelingIpsAndHostnamesWindowItem::SplitTunnelingIpsAndHostnamesWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage(parent)
    , preferences_(preferences)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    ipsAndHostnamesItem_ = new SplitTunnelingIpsAndHostnamesItem(this);
    connect(ipsAndHostnamesItem_, SIGNAL(networkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute>)), SLOT(onNetworkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute>)));
    connect(ipsAndHostnamesItem_, SIGNAL(nativeInfoErrorMessage(QString,QString)), SIGNAL(nativeInfoErrorMessage(QString,QString)));
    connect(ipsAndHostnamesItem_, SIGNAL(scrollToRect(QRect)), SIGNAL(scrollToRect(QRect)));
    connect(ipsAndHostnamesItem_, SIGNAL(escape()), SIGNAL(escape()));
    addItem(ipsAndHostnamesItem_);
    setFocusProxy(ipsAndHostnamesItem_);
    ipsAndHostnamesItem_->setNetworkRoutes(preferences->splitTunnelingNetworkRoutes());
}

QString SplitTunnelingIpsAndHostnamesWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "IPs & Hostnames");
}

void SplitTunnelingIpsAndHostnamesWindowItem::setLoggedIn(bool loggedIn)
{
    ipsAndHostnamesItem_->setLoggedIn(loggedIn);
}

void SplitTunnelingIpsAndHostnamesWindowItem::setFocusOnTextEntry()
{
    ipsAndHostnamesItem_->setFocusOnTextEntry();
}

void SplitTunnelingIpsAndHostnamesWindowItem::onNetworkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute> routes)
{
    preferences_->setSplitTunnelingNetworkRoutes(routes);
    emit networkRoutesUpdated(routes); // tell other screens about change
}

} // namespace
