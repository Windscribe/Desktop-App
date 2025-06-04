#pragma once

#include <QGraphicsObject>
#include <QGraphicsView>
#include "aboutwindow/aboutwindowitem.h"
#include "accountwindow/accountwindowitem.h"
#include "advancedwindow/advancedwindowitem.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/accountinfo.h"
#include "commongraphics/resizebar.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/resizablewindow.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/scrollarea.h"
#include "connectionwindow/connectionwindowitem.h"
#include "dnsdomainswindow/dnsdomainswindowitem.h"
#include "generalwindow/generalwindowitem.h"
#include "helpwindow/helpwindowitem.h"
#include "lookandfeelwindow/lookandfeelwindowitem.h"
#include "networkoptionswindow/networkoptionswindowitem.h"
#include "networkoptionswindow/networkoptionsnetworkwindowitem.h"
#include "preferencestab/preferencestabcontrolitem.h"
#include "proxysettingswindow/proxysettingswindowitem.h"
#include "robertwindow/robertwindowitem.h"
#include "splittunnelingwindow/splittunnelingwindowitem.h"
#include "splittunnelingwindow/splittunnelingaddresseswindowitem.h"
#include "splittunnelingwindow/splittunnelingappswindowitem.h"

namespace PreferencesWindow {

class PreferencesWindowItem : public ResizableWindow
{
    Q_OBJECT
public:
    explicit PreferencesWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper, AccountInfo *accountInfo);
    ~PreferencesWindowItem();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    PREFERENCES_TAB_TYPE currentTab();
    void setCurrentTab(PREFERENCES_TAB_TYPE tab);
    void setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage);

    void setLoggedIn(bool loggedIn);
    void setConfirmEmailResult(bool bSuccess);
    void setSendLogResult(bool bSuccess);

    void updateNetworkState(types::NetworkInterface network);

    void addApplicationManually(QString filename);

    void setPacketSizeDetectionState(bool on);
    void showPacketSizeDetectionError(const QString &title, const QString &message);

    void setRobertFilters(const QVector<api_responses::RobertFilter> &filters);
    void setRobertFiltersError();
    void setSplitTunnelingActive(bool active);
    void setWebSessionCompleted();
    void setPreferencesImportCompleted();
    void setLocationNamesImportCompleted();

signals:
    void logoutClick();
    void loginClick();
    void quitAppClick();

    void viewLogClick();
    void exportSettingsClick();
    void importSettingsClick();
    void exportLocationNamesClick();
    void importLocationNamesClick();
    void resetLocationNamesClick();
    void sendConfirmEmailClick();
    void sendDebugLogClick();
    void accountLoginClick();
    void manageAccountClick();
    void addEmailButtonClick();
    void manageRobertRulesClick();

    void currentNetworkUpdated(types::NetworkInterface);
    void cycleMacAddressClick();
    void splitTunnelingAppsAddButtonClick();
    void detectPacketSizeClick();

    void advancedParametersClicked();

    void getRobertFilters();
    void setRobertFilter(const api_responses::RobertFilter &filter);

public slots:
    virtual void onWindowExpanded() override;
    virtual void onWindowCollapsed() override;

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
    void onDnsDomainsEscape();
    void onNetworkEscape();

    void onCurrentNetworkUpdated(types::NetworkInterface network);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void updatePositions() override;

protected slots:
    void onBackArrowButtonClicked() override;

private:
    static constexpr int kTabAreaWidth = 64;
    static constexpr int kMinHeight = 596;

    PreferencesTabControlItem *tabControlItem_;
    GeneralWindowItem *generalWindowItem_;
    AccountWindowItem *accountWindowItem_;
    ConnectionWindowItem *connectionWindowItem_;
    RobertWindowItem *robertWindowItem_;
    LookAndFeelWindowItem *lookAndFeelWindowItem_;
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
