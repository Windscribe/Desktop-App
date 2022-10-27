#ifndef PREFERENCESWINDOW_H
#define PREFERENCESWINDOW_H

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
#include "backend/preferences/preferences.h"
#include "backend/preferences/accountinfo.h"
#include "commongraphics/resizebar.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/scrollarea.h"
#include "preferencestab/ipreferencestabcontrol.h"

namespace PreferencesWindow {


class PreferencesWindowItem : public ScalableGraphicsObject, public IPreferencesWindow
{
    Q_OBJECT
    Q_INTERFACES(IPreferencesWindow)
public:
    explicit PreferencesWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper, AccountInfo *accountInfo);
    ~PreferencesWindowItem() override;

    QGraphicsObject *getGraphicsObject() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    int minimumHeight() override;
    void setHeight(int height) override;

    void setCurrentTab(PREFERENCES_TAB_TYPE tab) override;
    void setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage ) override;

    void setScrollBarVisibility(bool on) override;

    void setLoggedIn(bool loggedIn) override;
    void setConfirmEmailResult(bool bSuccess) override;
    void setSendLogResult(bool bSuccess) override;

    void updateNetworkState(types::NetworkInterface network) override;

    void addApplicationManually(QString filename) override;

    void updateScaling() override;

    void setPacketSizeDetectionState(bool on) override;
    void showPacketSizeDetectionError(const QString &title, const QString &message) override;

    void setRobertFilters(const QVector<types::RobertFilter> &filters) override;
    void setRobertFiltersError() override;

    void setScrollOffset(int offset) override;

    void setSplitTunnelingActive(bool active) override;

signals:
    void escape() override;
    void signOutClick() override;
    void loginClick() override;
    void quitAppClick() override;
    void sizeChanged() override;
    void resizeFinished() override;

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
    void onResizeStarted();
    void onResizeChange(int y);
    void onCurrentTabChanged(PREFERENCES_TAB_TYPE tab);

    void onBackArrowButtonClicked();

    void onNetworkOptionsPageClick();
    void onNetworkOptionsNetworkClick(types::NetworkInterface network);
    void onSplitTunnelingPageClick();
    void onProxySettingsPageClick();

    void onAdvParametersClick();

    void onSplitTunnelingAppsClick();
    void onSplitTunnelingAddressesClick();
    void onAppsWindowAppsUpdated(QList<types::SplitTunnelingApp> apps);
    void onAddressesUpdated(QList<types::SplitTunnelingNetworkRoute> addresses);

    void onStAppsEscape();
    void onIpsAndHostnameEscape();
    void onNetworkEscape();

    void onCurrentNetworkUpdated(types::NetworkInterface network);

    void onAppSkinChanged(APP_SKIN s);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    static constexpr int BOTTOM_AREA_HEIGHT = 16;
    static constexpr int MIN_HEIGHT = 572;
    static constexpr int MIN_HEIGHT_VAN_GOGH = 544;

    static constexpr int BOTTOM_RESIZE_ORIGIN_X = 155;
    static constexpr int BOTTOM_RESIZE_OFFSET_Y = 13;

    static constexpr int SCROLL_AREA_WIDTH = 268;
    static constexpr int TAB_AREA_WIDTH = 64;

    Preferences *preferences_;

    int curHeight_;
    double curScale_;
    int heightAtResizeStart_;

    IconButton *backArrowButton_;

    CommonGraphics::ResizeBar *bottomResizeItem_;
    CommonGraphics::EscapeButton *escapeButton_;

    IPreferencesTabControl *tabControlItem_;
    CommonGraphics::ScrollArea *scrollAreaItem_;
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

    bool isShowSubPage_;
    QString pageCaption_;

    QString backgroundBase_;
    QString backgroundHeader_;

    bool roundedFooter_;
    QColor footerColor_;

    bool loggedIn_;

    void changeTab(PREFERENCES_TAB_TYPE tab);
    QRectF getBottomResizeArea();
    void moveOnePageBack();
    void setPreferencesWindowToSplitTunnelingAppsHome();
    void setPreferencesWindowToSplitTunnelingHome();
    void setShowSubpageMode(bool isShowSubPage);
    void updateChildItemsAfterHeightChanged();
    void updatePositions();
    void updateSplitTunnelingAppsCount(QList<types::SplitTunnelingApp> apps);
};

} // namespace PreferencesWindow

#endif // PREFERENCESWINDOW_H
