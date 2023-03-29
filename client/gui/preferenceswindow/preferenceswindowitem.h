#pragma once

#include <QGraphicsObject>
#include <QGraphicsView>
#include "ipreferenceswindow.h"
#include "aboutwindow/aboutwindowitem.h"
#include "accountwindow/accountwindowitem.h"
#include "advancedwindow/advancedwindowitem.h"
#include "connectionwindow/connectionwindowitem.h"
#include "generalwindow/generalwindowitem.h"
#include "helpwindow/helpwindowitem.h"
#include "networkoptionswindow/networkoptionswindowitem.h"
#include "networkoptionswindow/networkoptionsnetworkwindowitem.h"
#include "proxysettingswindow/proxysettingswindowitem.h"
#include "robertwindow/robertwindowitem.h"
#include "splittunnelingwindow/splittunnelingwindowitem.h"
#include "splittunnelingwindow/splittunnelingaddresseswindowitem.h"
#include "splittunnelingwindow/splittunnelingappswindowitem.h"
#include "dnsdomainswindow/dnsdomainswindowitem.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/accountinfo.h"
#include "commongraphics/resizebar.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/scrollarea.h"
#include "preferencestab/ipreferencestabcontrol.h"

namespace PreferencesWindow {


class PreferencesWindowItem : public IPreferencesWindow
{
    Q_OBJECT
    Q_INTERFACES(IPreferencesWindow)
public:
    explicit PreferencesWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper, AccountInfo *accountInfo);
    ~PreferencesWindowItem() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setCurrentTab(PREFERENCES_TAB_TYPE tab) override;
    void setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage ) override;

    void setLoggedIn(bool loggedIn) override;
    void setConfirmEmailResult(bool bSuccess) override;
    void setSendLogResult(bool bSuccess) override;

    void updateNetworkState(types::NetworkInterface network) override;

    void addApplicationManually(QString filename) override;

    void setPacketSizeDetectionState(bool on) override;
    void showPacketSizeDetectionError(const QString &title, const QString &message) override;

    void setRobertFilters(const QVector<types::RobertFilter> &filters) override;
    void setRobertFiltersError() override;

    void setSplitTunnelingActive(bool active) override;

    void onCollapse() override;

signals:
    void signOutClick() override;
    void loginClick() override;
    void quitAppClick() override;

    void viewLogClick() override;
    void sendConfirmEmailClick() override;
    void sendDebugLogClick() override;
    void accountLoginClick() override;
    void manageAccountClick() override;
    void addEmailButtonClick() override;
    void manageRobertRulesClick() override;

    void currentNetworkUpdated(types::NetworkInterface) override;
    void cycleMacAddressClick();
    void splitTunnelingAppsAddButtonClick();
    void detectPacketSizeClick();

    void advancedParametersClicked() override;

    void getRobertFilters();
    void setRobertFilter(const types::RobertFilter &filter);

#ifdef Q_OS_WIN
    void setIpv6StateInOS(bool bEnabled, bool bRestartNow) override;
#endif

private slots:
    void onCurrentTabChanged(PREFERENCES_TAB_TYPE tab);

    void onNetworkOptionsPageClick();
    void onNetworkOptionsNetworkClick(types::NetworkInterface network);
    void onSplitTunnelingPageClick();
    void onProxySettingsPageClick();
    void onConnectedDnsDomainsClick(const QStringList &domains);

    void onAdvParametersClick();

    void onSplitTunnelingAppsClick();
    void onSplitTunnelingAddressesClick();
    void onAppsWindowAppsUpdated(QList<types::SplitTunnelingApp> apps);
    void onAddressesUpdated(QList<types::SplitTunnelingNetworkRoute> addresses);

    void onStAppsEscape();
    void onIpsAndHostnameEscape();
    void onNetworkEscape();

    void onCurrentNetworkUpdated(types::NetworkInterface network);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void updatePositions() override;
    void updateChildItemsAfterHeightChanged() override;

protected slots:
    void onBackArrowButtonClicked() override;

private:
    static constexpr int kTabAreaWidth = 64;
    static constexpr int kMinHeight = 572;

    IPreferencesTabControl *tabControlItem_;
    GeneralWindowItem *generalWindowItem_;
    AccountWindowItem *accountWindowItem_;
    ConnectionWindowItem *connectionWindowItem_;
    RobertWindowItem *robertWindowItem_;
    AdvancedWindowItem *advancedWindowItem_;
    HelpWindowItem *helpWindowItem_;
    AboutWindowItem *aboutWindowItem_;

    // sub screens
    NetworkOptionsWindowItem *networkOptionsWindowItem_;
    NetworkOptionsNetworkWindowItem *networkOptionsNetworkWindowItem_;
    ProxySettingsWindowItem *proxySettingsWindowItem_;
    SplitTunnelingWindowItem *splitTunnelingWindowItem_;
    SplitTunnelingAppsWindowItem *splitTunnelingAppsWindowItem_;
    SplitTunnelingAddressesWindowItem *splitTunnelingAddressesWindowItem_;
    DnsDomainsWindowItem *dnsDomainsWindowItem_;

    bool isShowSubPage_;
    bool loggedIn_;

    void changeTab(PREFERENCES_TAB_TYPE tab);
    void moveOnePageBack();
    void setPreferencesWindowToSplitTunnelingAppsHome();
    void setPreferencesWindowToSplitTunnelingHome();
    void setShowSubpageMode(bool isShowSubPage);
    void updateSplitTunnelingAppsCount(QList<types::SplitTunnelingApp> apps);
};

} // namespace PreferencesWindow
