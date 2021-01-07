#ifndef PREFERENCESWINDOW_H
#define PREFERENCESWINDOW_H

#include <QGraphicsObject>
#include <QGraphicsView>
#include "ipreferenceswindow.h"
#include "bottomresizeitem.h"
#include "scrollareaitem.h"
#include "generalwindow/generalwindowitem.h"
#include "accountwindow/accountwindowitem.h"
#include "connectionwindow/connectionwindowitem.h"
#include "sharewindow/sharewindowitem.h"
#include "debugwindow/debugwindowitem.h"
#include "networkwhitelistwindow/networkwhitelistwindowitem.h"
#include "splittunnelingwindow/splittunnelingwindowitem.h"
#include "splittunnelingwindow/splittunnelingiphostnamewindowitem.h"
#include "splittunnelingwindow/splittunnelingappswindowitem.h"
#include "splittunnelingwindow/splittunnelingappssearchwindowitem.h"
#include "proxysettingswindow/proxysettingswindowitem.h"
#include "debugwindow/advancedparameterswindowitem.h"
#include "escapebutton.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/accountinfo.h"
#include "preferencestab/ipreferencestabcontrol.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"

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

    int recommendedHeight() override;
    void setHeight(int height) override;

    void setCurrentTab(PREFERENCES_TAB_TYPE tab) override;
    void setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage ) override;

    void setScrollBarVisibility(bool on) override;

    void setLoggedIn(bool loggedIn) override;
    void setConfirmEmailResult(bool bSuccess) override;
    void setDebugLogResult(bool bSuccess) override;

    void updateNetworkState(ProtoTypes::NetworkInterface network) override;

    void setPreferencesWindowToSplitTunnelingHome();
    void setPreferencesWindowToSplitTunnelingAppsHome();
    void setPreferencesWindowToSplitTunnelingAppsSearch();
    void addApplicationManually(QString filename) override;

    void updateScaling() override;
    void updatePageSpecific() override;

    void setPacketSizeDetectionState(bool on) override;
    void showPacketSizeDetectionError(const QString &title, const QString &message) override;

signals:
    void escape() override;
    void helpClick() override;
    void signOutClick() override;
    void loginClick() override;
    void quitAppClick() override;
    void sizeChanged() override;
    void resizeFinished() override;

    void viewLogClick() override;
    void sendConfirmEmailClick() override;
    void sendDebugLogClick() override;
    void noAccountLoginClick() override;

    void currentNetworkUpdated(ProtoTypes::NetworkInterface) override;
    void cycleMacAddressClick();
    void nativeInfoErrorMessage(QString title, QString desc);
    void splitTunnelingAppsAddButtonClick();
    void detectAppropriatePacketSizeButtonClicked();

    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

    void advancedParametersClicked() override;

#ifdef Q_OS_WIN
    void setIpv6StateInOS(bool bEnabled, bool bRestartNow) override;
#endif

private slots:
    void onResizeStarted();
    void onResizeChange(int y);
    void onCurrentTabChanged(PREFERENCES_TAB_TYPE tab);

    void onBackArrowButtonClicked();

    void onNetworkWhitelistPageClick();
    void onSplitTunnelingPageClick();
    void onProxySettingsPageClick();

    void onAdvParametersClick();

    void onSplitTunnelingAppsClick();
    void onSplitTunnelingAppsSearchClick();
    void onSplitTunnelingIpsAndHostnamesClick();
    void onAppsSearchWindowAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps);
    void onAppsWindowAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps);
    void onNetworkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute> routes);
    void onSearchModeExit();

    void onStAppsEscape();
    void onStAppsSearchEscape();
    void onIpsAndHostnameEscape();

    void onCurrentNetworkUpdated(ProtoTypes::NetworkInterface network);

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void changeTab(PREFERENCES_TAB_TYPE tab);

    static constexpr int BOTTOM_AREA_HEIGHT = 17;
    static constexpr int MIN_HEIGHT = 502;

    static constexpr int BOTTOM_RESIZE_ORIGIN_X = 167;
    static constexpr int BOTTOM_RESIZE_OFFSET_Y = 13;

    BottomResizeItem *bottomResizeItem_;
    int curHeight_;
    double curScale_;
    int heightAtResizeStart_;

    IconButton *backArrowButton_;

    EscapeButton *escapeButton_;

    IPreferencesTabControl *tabControlItem_;
    ScrollAreaItem *scrollAreaItem_;
    GeneralWindowItem *generalWindowItem_;
    AccountWindowItem *accountWindowItem_;
    ConnectionWindowItem *connectionWindowItem_;
    ShareWindowItem *shareWindowItem_;
    DebugWindowItem *debugWindowItem_;

    // sub screens
    // AdvancedParametersWindowItem *advancedParametersWindowItem_;
    NetworkWhiteListWindowItem *networkWhiteListWindowItem_;
    ProxySettingsWindowItem *proxySettingsWindowItem_;
    SplitTunnelingWindowItem *splitTunnelingWindowItem_;
    SplitTunnelingAppsWindowItem *splitTunnelingAppsWindowItem_;
    SplitTunnelingAppsSearchWindowItem *splitTunnelingAppsSearchWindowItem_;
    SplitTunnelingIpsAndHostnamesWindowItem *splitTunnelingIpsAndHostnamesWindowItem_;

    bool isShowSubPage_;
    QString pageCaption_;

    QString backgroundBase_;
    QString backgroundHeader_;

    bool roundedFooter_;
    QColor footerColor_;

    void moveOnePageBack();

    void setShowSubpageMode(bool isShowSubPage);
    QRectF getBottomResizeArea();
    void updateChildItemsAfterHeightChanged();

    void updateSplitTunnelingAppsCount(QList<ProtoTypes::SplitTunnelingApp> apps);
    void updatePositions();
};

} // namespace PreferencesWindow

#endif // PREFERENCESWINDOW_H
