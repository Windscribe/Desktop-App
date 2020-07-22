#ifndef PREFERENCESWINDOW_H
#define PREFERENCESWINDOW_H

#include <QGraphicsObject>
#include <QGraphicsView>
#include "ipreferenceswindow.h"
#include "bottomresizeitem.h"
#include "scrollareaitem.h"
#include "GeneralWindow/generalwindowitem.h"
#include "AccountWindow/accountwindowitem.h"
#include "ConnectionWindow/connectionwindowitem.h"
#include "ShareWindow/sharewindowitem.h"
#include "DebugWindow/debugwindowitem.h"
#include "NetworkWhitelistWindow/networkwhitelistwindowitem.h"
#include "SplitTunnelingWindow/splittunnelingwindowitem.h"
#include "SplitTunnelingWindow/splittunnelingiphostnamewindowitem.h"
#include "SplitTunnelingWindow/splittunnelingappswindowitem.h"
#include "SplitTunnelingWindow/splittunnelingappssearchwindowitem.h"
#include "ProxySettingsWindow/proxysettingswindowitem.h"
#include "DebugWindow/advancedparameterswindowitem.h"
#include "escapebutton.h"
#include "../Backend/Preferences/preferences.h"
#include "../Backend/Preferences/accountinfo.h"
#include "PreferencesTab/ipreferencestabcontrol.h"
#include "CommonGraphics/iconbutton.h"
#include "CommonGraphics/scalablegraphicsobject.h"
#include "IPC/generated_proto/types.pb.h"

namespace PreferencesWindow {


class PreferencesWindowItem : public ScalableGraphicsObject, public IPreferencesWindow
{
    Q_OBJECT
    Q_INTERFACES(IPreferencesWindow)
public:
    explicit PreferencesWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper, AccountInfo *accountInfo);
    virtual ~PreferencesWindowItem() override;

    virtual QGraphicsObject *getGraphicsObject() override;

    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    virtual int recommendedHeight() override;
    virtual void setHeight(int height) override;

    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab) override;
    virtual void setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage ) override;

    virtual void setScrollBarVisibility(bool on) override;

    virtual void setLoggedIn(bool loggedIn) override;
    virtual void setConfirmEmailResult(bool bSuccess) override;
    virtual void setDebugLogResult(bool bSuccess) override;

    void updateNetworkState(ProtoTypes::NetworkInterface network) override;

    void setPreferencesWindowToSplitTunnelingHome();
    void setPreferencesWindowToSplitTunnelingAppsHome();
    void setPreferencesWindowToSplitTunnelingAppsSearch();
    void addApplicationManually(QString filename) override;

    void updateScaling() override;
    void updatePageSpecific() override;

    void setPacketSizeDetectionState(bool on) override;

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
    void clearServerRatingsClick() override;

    void currentNetworkUpdated(ProtoTypes::NetworkInterface) override;
    void cycleMacAddressClick();
    void nativeInfoErrorMessage(QString title, QString desc);
    void splitTunnelingAppsAddButtonClick();
    void detectPacketMssButtonClicked();

    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

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
    void keyPressEvent(QKeyEvent *event) override;

private:
    void changeTab(PREFERENCES_TAB_TYPE tab);

    const int BOTTOM_AREA_HEIGHT = 17;
    const int MIN_HEIGHT = 502;

    const int bottomResizeOriginX_ = 167;
    const int bottomResizeOffsetY_ = 13;

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
    AdvancedParametersWindowItem *advancedParametersWindowItem_;
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
