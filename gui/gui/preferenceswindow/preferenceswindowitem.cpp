#include "preferenceswindowitem.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferencestab/preferencestabcontrolitem.h"
#include "graphicresources/fontmanager.h"
#include <QPainter>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include "utils/utils.h"
#include "dpiscalemanager.h"


namespace PreferencesWindow {


PreferencesWindowItem::PreferencesWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper, AccountInfo *accountInfo) : ScalableGraphicsObject(parent),
    curScale_(1.0), isShowSubPage_(false)
{
    setFlags(QGraphicsObject::ItemIsFocusable);
    installEventFilter(this);

    curHeight_ = (MIN_HEIGHT - 100);

#ifdef Q_OS_WIN
    backgroundBase_ = "background/WIN_TOP_BG";
    backgroundHeader_ = "background/WIN_HEADER_BG_OVERLAY";
    roundedFooter_ = false;
#else
    backgroundBase_ = "background/MAC_TOP_BG";
    backgroundHeader_ = "background/MAC_HEADER_BG_OVERLAY";
    roundedFooter_ = true;
#endif
    footerColor_ = FontManager::instance().getCharcoalColor();

    escapeButton_ = new EscapeButton(this);
    connect(escapeButton_, SIGNAL(clicked()), SIGNAL(escape()));

    tabControlItem_ = new PreferencesTabControlItem(this);
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(currentTabChanged(PREFERENCES_TAB_TYPE)), SLOT(onCurrentTabChanged(PREFERENCES_TAB_TYPE)));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(helpClick()), SIGNAL(helpClick()));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(signOutClick()), SIGNAL(signOutClick()));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(loginClick()), SIGNAL(loginClick()));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(quitClick()), SIGNAL(quitAppClick()));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(showTooltip(TooltipInfo)), SIGNAL(showTooltip(TooltipInfo)));
    connect(dynamic_cast<QObject*>(tabControlItem_), SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));

    backArrowButton_ = new IconButton(20, 24, "login/BACK_ARROW", this);
    // backArrowButton_->hide();
    connect(backArrowButton_, SIGNAL(clicked()), SLOT(onBackArrowButtonClicked()));

    bottomResizeItem_ = new BottomResizeItem(this);
    connect(bottomResizeItem_, SIGNAL(resizeStarted()), SLOT(onResizeStarted()));
    connect(bottomResizeItem_, SIGNAL(resizeChange(int)), SLOT(onResizeChange(int)));
    connect(bottomResizeItem_, SIGNAL(resizeFinished()), SIGNAL(resizeFinished()));

    scrollAreaItem_ = new ScrollAreaItem(this, curHeight_  - 102 );

    generalWindowItem_ = new GeneralWindowItem(nullptr, preferences, preferencesHelper);
    accountWindowItem_ = new AccountWindowItem(nullptr, accountInfo);
    accountWindowItem_->setLoggedIn(false);
    connect(accountWindowItem_, SIGNAL(sendConfirmEmailClick()), SIGNAL(sendConfirmEmailClick()));
    connect(accountWindowItem_, SIGNAL(noAccountLoginClick()), SIGNAL(noAccountLoginClick()));

    connectionWindowItem_ = new ConnectionWindowItem(nullptr, preferences, preferencesHelper);
    shareWindowItem_ = new ShareWindowItem(nullptr, preferences, preferencesHelper);
    debugWindowItem_ = new DebugWindowItem(nullptr, preferences, preferencesHelper);

    connect(connectionWindowItem_, SIGNAL(networkWhitelistPageClick()), SLOT(onNetworkWhitelistPageClick()));
    connect(connectionWindowItem_, SIGNAL(splitTunnelingPageClick()), SLOT(onSplitTunnelingPageClick()));
    connect(connectionWindowItem_, SIGNAL(proxySettingsPageClick()), SLOT(onProxySettingsPageClick()));
    connect(connectionWindowItem_, SIGNAL(cycleMacAddressClick()), SIGNAL(cycleMacAddressClick()));
    connect(connectionWindowItem_, SIGNAL(detectAppropriatePacketSizeButtonClicked()), SIGNAL(detectAppropriatePacketSizeButtonClicked()));
    connect(connectionWindowItem_, SIGNAL(showTooltip(TooltipInfo)), SIGNAL(showTooltip(TooltipInfo)));
    connect(connectionWindowItem_, SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));

    connect(debugWindowItem_, SIGNAL(viewLogClick()), SIGNAL(viewLogClick()));
    connect(debugWindowItem_, SIGNAL(sendLogClick()), SIGNAL(sendDebugLogClick()));
    connect(debugWindowItem_, SIGNAL(advParametersClick()), SLOT(onAdvParametersClick()));
    connect(debugWindowItem_, SIGNAL(showTooltip(TooltipInfo)), SIGNAL(showTooltip(TooltipInfo)));
    connect(debugWindowItem_, SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));

    connect(shareWindowItem_, SIGNAL(showTooltip(TooltipInfo)), SIGNAL(showTooltip(TooltipInfo)));
    connect(shareWindowItem_, SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));
#ifdef Q_OS_WIN
    connect(debugWindowItem_, SIGNAL(setIpv6StateInOS(bool,bool)), SIGNAL(setIpv6StateInOS(bool,bool)));
#endif
    scrollAreaItem_->setItem(generalWindowItem_);
    pageCaption_ = generalWindowItem_->caption();


    networkWhiteListWindowItem_ = new NetworkWhiteListWindowItem(nullptr, preferences);
    proxySettingsWindowItem_ = new ProxySettingsWindowItem(nullptr, preferences);
    advancedParametersWindowItem_ = new AdvancedParametersWindowItem(nullptr, preferences);
    splitTunnelingWindowItem_ = new SplitTunnelingWindowItem(nullptr, preferences);
    splitTunnelingAppsWindowItem_ = new SplitTunnelingAppsWindowItem(nullptr, preferences);
    splitTunnelingAppsSearchWindowItem_ = new SplitTunnelingAppsSearchWindowItem(nullptr, preferences);
    splitTunnelingIpsAndHostnamesWindowItem_ = new SplitTunnelingIpsAndHostnamesWindowItem(nullptr, preferences);

    connect(splitTunnelingWindowItem_, SIGNAL(appsPageClick()), SLOT(onSplitTunnelingAppsClick()));
    connect(splitTunnelingWindowItem_, SIGNAL(ipsAndHostnamesPageClick()), SLOT(onSplitTunnelingIpsAndHostnamesClick()));
    connect(splitTunnelingWindowItem_, SIGNAL(showTooltip(TooltipInfo)), SIGNAL(showTooltip(TooltipInfo)));
    connect(splitTunnelingWindowItem_, SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));
    connect(splitTunnelingAppsWindowItem_, SIGNAL(searchButtonClicked()), SLOT(onSplitTunnelingAppsSearchClick()));
    connect(splitTunnelingAppsWindowItem_, SIGNAL(addButtonClicked()), SIGNAL(splitTunnelingAppsAddButtonClick()));
    connect(splitTunnelingAppsWindowItem_, SIGNAL(appsUpdated(QList<ProtoTypes::SplitTunnelingApp>)), SLOT(onAppsWindowAppsUpdated(QList<ProtoTypes::SplitTunnelingApp>)));
    connect(splitTunnelingAppsWindowItem_, SIGNAL(nativeInfoErrorMessage(QString,QString)), SIGNAL(nativeInfoErrorMessage(QString,QString)));
    connect(splitTunnelingAppsWindowItem_, SIGNAL(escape()), SLOT(onStAppsEscape()));

    connect(splitTunnelingAppsSearchWindowItem_, SIGNAL(appsUpdated(QList<ProtoTypes::SplitTunnelingApp>)), SLOT(onAppsSearchWindowAppsUpdated(QList<ProtoTypes::SplitTunnelingApp>)));
    connect(splitTunnelingAppsSearchWindowItem_, SIGNAL(searchModeExited()), SLOT(onSearchModeExit()));
    connect(splitTunnelingAppsSearchWindowItem_, SIGNAL(nativeInfoErrorMessage(QString,QString)), SIGNAL(nativeInfoErrorMessage(QString,QString)));
    connect(splitTunnelingAppsSearchWindowItem_, SIGNAL(escape()), SLOT(onStAppsSearchEscape()));

    connect(splitTunnelingIpsAndHostnamesWindowItem_, SIGNAL(networkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute>)), SLOT(onNetworkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute>)));
    connect(splitTunnelingIpsAndHostnamesWindowItem_, SIGNAL(nativeInfoErrorMessage(QString,QString)), SIGNAL(nativeInfoErrorMessage(QString,QString)));
    connect(splitTunnelingIpsAndHostnamesWindowItem_, SIGNAL(escape()), SLOT(onIpsAndHostnameEscape()));

    connect(networkWhiteListWindowItem_, SIGNAL(currentNetworkUpdated(ProtoTypes::NetworkInterface)), SLOT(onCurrentNetworkUpdated(ProtoTypes::NetworkInterface)));
    updatePositions();
}

PreferencesWindowItem::~PreferencesWindowItem()
{
    delete advancedParametersWindowItem_;
    delete networkWhiteListWindowItem_;
    delete proxySettingsWindowItem_;
    delete splitTunnelingWindowItem_;
    delete splitTunnelingAppsWindowItem_;
    delete splitTunnelingAppsSearchWindowItem_;
    delete splitTunnelingIpsAndHostnamesWindowItem_;

    delete generalWindowItem_;
    delete accountWindowItem_;
    delete connectionWindowItem_;
    delete shareWindowItem_;
    delete debugWindowItem_;
}

QGraphicsObject *PreferencesWindowItem::getGraphicsObject()
{
    return this;
}

QRectF PreferencesWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH * G_SCALE, curHeight_);
}

void PreferencesWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // resize area background
    qreal initialOpacity = painter->opacity();
    painter->fillRect(boundingRect().adjusted(0, 286*G_SCALE, 0, -7*G_SCALE), QBrush(QColor(2, 13, 28)));

    // base background
    IndependentPixmap *pixmapBaseBackground = ImageResourcesSvg::instance().getIndependentPixmap(backgroundBase_);
    pixmapBaseBackground->draw(0, 0, painter);
    IndependentPixmap *pixmapHeader = ImageResourcesSvg::instance().getIndependentPixmap(backgroundHeader_);
    pixmapHeader->draw(0, 27*G_SCALE, painter);

    // draw page caption
    QRect rcCaption(56*G_SCALE, 30*G_SCALE, 200*G_SCALE, 56*G_SCALE);
    //painter->fillRect(rcCaption, QBrush(QColor(255, 45, 61)));
    painter->setPen(QColor(255, 255, 255));
    QFont *font = FontManager::instance().getFont(16, true);
    painter->setFont(*font);
    painter->drawText(rcCaption, Qt::AlignLeft | Qt::AlignVCenter, tr(pageCaption_.toStdString().c_str()));

    // vertical line divider between tab selector and pages
    painter->setOpacity(0.15 * initialOpacity);
    painter->fillRect(QRect(48*G_SCALE, 83*G_SCALE, 2*G_SCALE, boundingRect().height() - 100*G_SCALE), QBrush(Qt::white));

    // bottom-most background
    painter->setOpacity(initialOpacity);
    if (roundedFooter_)
    {
        painter->setPen(footerColor_);
        painter->setBrush(footerColor_);
        painter->drawRoundedRect(getBottomResizeArea(), 8 * G_SCALE, 8 * G_SCALE);
        painter->fillRect(getBottomResizeArea().adjusted(0, -2* G_SCALE, 0, -7*G_SCALE), QBrush(footerColor_));
    }
    else
    {
        painter->fillRect(getBottomResizeArea(), QBrush(footerColor_));
    }
}

int PreferencesWindowItem::recommendedHeight()
{
    return MIN_HEIGHT;
}

void PreferencesWindowItem::setHeight(int height)
{
    prepareGeometryChange();
    curHeight_ = height;
    updateChildItemsAfterHeightChanged();
}

void PreferencesWindowItem::setCurrentTab(PREFERENCES_TAB_TYPE tab)
{
    changeTab(tab);
    tabControlItem_->setCurrentTab(tab);
}

void PreferencesWindowItem::setCurrentTab(PREFERENCES_TAB_TYPE tab, CONNECTION_SCREEN_TYPE subpage)
{
    changeTab(tab);

    // set subpage
    if (tab == TAB_CONNECTION)
    {
        if (subpage == CONNECTION_SCREEN_NETWORK_WHITELIST)
        {
            tabControlItem_->setCurrentTab(TAB_CONNECTION);
            connectionWindowItem_->setScreen(CONNECTION_SCREEN_NETWORK_WHITELIST);
            onNetworkWhitelistPageClick();
        }
        else if (subpage == CONNECTION_SCREEN_SPLIT_TUNNELING)
        {
            tabControlItem_->setCurrentTab(TAB_CONNECTION);
            connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
            setPreferencesWindowToSplitTunnelingHome();
        }
    }
}

void PreferencesWindowItem::setScrollBarVisibility(bool on)
{
    scrollAreaItem_->setScrollBarVisibility(on);
}

void PreferencesWindowItem::setLoggedIn(bool loggedIn)
{
    tabControlItem_->setLoggedIn(loggedIn);
    accountWindowItem_->setLoggedIn(loggedIn);
    splitTunnelingAppsWindowItem_->setLoggedIn(loggedIn);
    splitTunnelingAppsSearchWindowItem_->setLoggedIn(loggedIn);
    splitTunnelingIpsAndHostnamesWindowItem_->setLoggedIn(loggedIn);
}

void PreferencesWindowItem::setConfirmEmailResult(bool bSuccess)
{
    accountWindowItem_->setConfirmEmailResult(bSuccess);
}

void PreferencesWindowItem::setDebugLogResult(bool bSuccess)
{
    debugWindowItem_->setDebugLogResult(bSuccess);
}

void PreferencesWindowItem::updateNetworkState(ProtoTypes::NetworkInterface network)
{
    networkWhiteListWindowItem_->setCurrentNetwork(network);
    connectionWindowItem_->setCurrentNetwork(network);
}

void PreferencesWindowItem::onResizeStarted()
{
    heightAtResizeStart_ = curHeight_;
}

void PreferencesWindowItem::onResizeChange(int y)
{
    if ((heightAtResizeStart_ + y) >= MIN_HEIGHT*G_SCALE)
    {
        prepareGeometryChange();
        curHeight_ = heightAtResizeStart_ + y;
        updateChildItemsAfterHeightChanged();

        emit sizeChanged();
    }
}

void PreferencesWindowItem::changeTab(PREFERENCES_TAB_TYPE tab)
{
    if (tab == TAB_GENERAL)
    {
        scrollAreaItem_->setItem(generalWindowItem_);
        generalWindowItem_->updateScaling();
        setShowSubpageMode(false);
        pageCaption_ = generalWindowItem_->caption();
        update();
    }
    else if (tab == TAB_ACCOUNT)
    {
        scrollAreaItem_->setItem(accountWindowItem_);
        accountWindowItem_->updateScaling();
        setShowSubpageMode(false);
        pageCaption_ = accountWindowItem_->caption();
        update();
    }
    else if (tab == TAB_CONNECTION)
    {
        scrollAreaItem_->setItem(connectionWindowItem_);
        connectionWindowItem_->updateScaling();
        connectionWindowItem_->setScreen(CONNECTION_SCREEN_HOME);
        setShowSubpageMode(false);
        pageCaption_ = connectionWindowItem_->caption();
        update();
    }
    else if (tab == TAB_SHARE)
    {
        scrollAreaItem_->setItem(shareWindowItem_);
        shareWindowItem_->updateScaling();
        setShowSubpageMode(false);
        pageCaption_ = shareWindowItem_->caption();
        update();
    }
    else if (tab == TAB_DEBUG)
    {
        scrollAreaItem_->setItem(debugWindowItem_);
        debugWindowItem_->updateScaling();
        debugWindowItem_->setScreen(DEBUG_SCREEN_HOME);
        setShowSubpageMode(false);
        pageCaption_ = debugWindowItem_->caption();
        update();
    }
    else
    {
        Q_ASSERT(false);
    }
}

void PreferencesWindowItem::onCurrentTabChanged(PREFERENCES_TAB_TYPE tab)
{
    changeTab(tab);
}

void PreferencesWindowItem::moveOnePageBack()
{
    PREFERENCES_TAB_TYPE currentTab = tabControlItem_->currentTab();

    if (currentTab == TAB_CONNECTION)
    {
        CONNECTION_SCREEN_TYPE screen = connectionWindowItem_->getScreen();

        if (screen == CONNECTION_SCREEN_NETWORK_WHITELIST)
        {
            changeTab(tabControlItem_->currentTab());
        }
        else if (screen == CONNECTION_SCREEN_SPLIT_TUNNELING)
        {
            SPLIT_TUNNEL_SCREEN splitTunnelScreen = splitTunnelingWindowItem_->getScreen();

            if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_HOME)
            {
                changeTab(tabControlItem_->currentTab());
            }
            else if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_APPS)
            {
                setPreferencesWindowToSplitTunnelingHome();
            }
            else if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_APPS_SEARCH)
            {
                setPreferencesWindowToSplitTunnelingAppsHome();
            }
            else if (splitTunnelScreen == SPLIT_TUNNEL_SCREEN_IPS_AND_HOSTNAMES)
            {
                setPreferencesWindowToSplitTunnelingHome();
            }
        }
        else if (screen == CONNECTION_SCREEN_PROXY_SETTINGS)
        {
            changeTab(tabControlItem_->currentTab());
        }
    }
    else //  non-connection screen
    {
        changeTab(tabControlItem_->currentTab());
    }
}

void PreferencesWindowItem::onBackArrowButtonClicked()
{
    if (isShowSubPage_)
    {
        moveOnePageBack();
    }
    else
    {
        emit escape();
    }
}

void PreferencesWindowItem::onNetworkWhitelistPageClick()
{
    scrollAreaItem_->setItem(networkWhiteListWindowItem_);
    networkWhiteListWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_NETWORK_WHITELIST);
    setShowSubpageMode(true);
    pageCaption_ = networkWhiteListWindowItem_->caption();
    setFocus();
    update();
}

void PreferencesWindowItem::setPreferencesWindowToSplitTunnelingHome()
{
    scrollAreaItem_->setItem(splitTunnelingWindowItem_);
    splitTunnelingWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
    splitTunnelingWindowItem_->setScreen(SPLIT_TUNNEL_SCREEN_HOME);
    setShowSubpageMode(true);
    pageCaption_ = splitTunnelingWindowItem_->caption();
    setFocus();
    update();
}

void PreferencesWindowItem::onSplitTunnelingPageClick()
{
    setPreferencesWindowToSplitTunnelingHome();
}

void PreferencesWindowItem::onProxySettingsPageClick()
{
    scrollAreaItem_->setItem(proxySettingsWindowItem_);
    proxySettingsWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_PROXY_SETTINGS);
    setShowSubpageMode(true);
    pageCaption_ = proxySettingsWindowItem_->caption();
    setFocus();
    update();
}

void PreferencesWindowItem::onAdvParametersClick()
{
    scrollAreaItem_->setItem(advancedParametersWindowItem_);
    advancedParametersWindowItem_->updateScaling();
    debugWindowItem_->setScreen(DEBUG_SCREEN_ADVANCED_PARAMETERS);
    setShowSubpageMode(true);
    pageCaption_ = advancedParametersWindowItem_->caption();
    update();
}

void PreferencesWindowItem::setPreferencesWindowToSplitTunnelingAppsHome()
{
    scrollAreaItem_->setItem(splitTunnelingAppsWindowItem_);
    splitTunnelingAppsWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
    splitTunnelingWindowItem_->setScreen(SPLIT_TUNNEL_SCREEN_APPS);
    setShowSubpageMode(true);
    pageCaption_ = splitTunnelingAppsWindowItem_->caption();
    update();
}

void PreferencesWindowItem::setPreferencesWindowToSplitTunnelingAppsSearch()
{
    scrollAreaItem_->setItem(splitTunnelingAppsSearchWindowItem_);
    splitTunnelingAppsSearchWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
    splitTunnelingWindowItem_->setScreen(SPLIT_TUNNEL_SCREEN_APPS_SEARCH);
    splitTunnelingAppsSearchWindowItem_->updateProgramList();
    setShowSubpageMode(true);
    pageCaption_ = splitTunnelingAppsSearchWindowItem_->caption();
    update();
    splitTunnelingAppsSearchWindowItem_->setFocusOnSearchBar();
}

void PreferencesWindowItem::addApplicationManually(QString filename)
{
    QList<ProtoTypes::SplitTunnelingApp> apps = splitTunnelingAppsWindowItem_->getApps();

    QString friendlyName = Utils::fileNameFromFullPath(filename);

    ProtoTypes::SplitTunnelingApp app;
    app.set_name(friendlyName.toStdString());
    app.set_active(true);
    app.set_type(ProtoTypes::SPLIT_TUNNELING_APP_TYPE_USER);
    app.set_full_name(filename.toStdString());
    apps.append(app);

    updateSplitTunnelingAppsCount(apps);
    splitTunnelingAppsWindowItem_->addAppManually(app); // handles setApps trickle down
    splitTunnelingAppsSearchWindowItem_->setApps(apps);
}

void PreferencesWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    curHeight_ = curHeight_ * (G_SCALE / curScale_);
    curScale_ = G_SCALE;

    updatePositions();
}

void PreferencesWindowItem::updatePageSpecific()
{
    if (connectionWindowItem_->getScreen() == CONNECTION_SCREEN_SPLIT_TUNNELING
        && splitTunnelingWindowItem_->getScreen() == SPLIT_TUNNEL_SCREEN_APPS_SEARCH)
    {
        splitTunnelingAppsSearchWindowItem_->updateProgramList();
    }
}

void PreferencesWindowItem::setPacketSizeDetectionState(bool on)
{
    connectionWindowItem_->setPacketSizeDetectionState(on);
}

void PreferencesWindowItem::showPacketSizeDetectionError(const QString &title,
                                                         const QString &message)
{
    connectionWindowItem_->showPacketSizeDetectionError(title, message);
}

void PreferencesWindowItem::onSplitTunnelingAppsClick()
{
    setPreferencesWindowToSplitTunnelingAppsHome();
}

void PreferencesWindowItem::onSplitTunnelingAppsSearchClick()
{
    setPreferencesWindowToSplitTunnelingAppsSearch();
}

void PreferencesWindowItem::onSplitTunnelingIpsAndHostnamesClick()
{
    scrollAreaItem_->setItem(splitTunnelingIpsAndHostnamesWindowItem_);
    splitTunnelingIpsAndHostnamesWindowItem_->updateScaling();
    connectionWindowItem_->setScreen(CONNECTION_SCREEN_SPLIT_TUNNELING);
    splitTunnelingWindowItem_->setScreen(SPLIT_TUNNEL_SCREEN_IPS_AND_HOSTNAMES);
    setShowSubpageMode(true);
    pageCaption_ = splitTunnelingIpsAndHostnamesWindowItem_->caption();
    update();
    splitTunnelingIpsAndHostnamesWindowItem_->setFocusOnTextEntry();
}

void PreferencesWindowItem::updateSplitTunnelingAppsCount(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    int activeApps = 0;
    foreach (ProtoTypes::SplitTunnelingApp app, apps)
    {
        if (app.active()) activeApps++;
    }
    splitTunnelingWindowItem_->setAppsCount(activeApps);
}

void PreferencesWindowItem::updatePositions()
{
    escapeButton_->setPos(WINDOW_WIDTH*G_SCALE - escapeButton_->boundingRect().width() - 16*G_SCALE, 16*G_SCALE);

    tabControlItem_->getGraphicsObject()->setPos(0, 82*G_SCALE);
    tabControlItem_->setHeight(curHeight_ - 99*G_SCALE);

    backArrowButton_->setPos(10*G_SCALE, 45*G_SCALE);
    bottomResizeItem_->setPos(BOTTOM_RESIZE_ORIGIN_X*G_SCALE, curHeight_ - BOTTOM_RESIZE_OFFSET_Y*G_SCALE);

    scrollAreaItem_->setHeight(curHeight_  - 102*G_SCALE);
    scrollAreaItem_->setPos(50*G_SCALE, 83*G_SCALE);
}

void PreferencesWindowItem::onAppsSearchWindowAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    updateSplitTunnelingAppsCount(apps);
    splitTunnelingAppsWindowItem_->setApps(apps);
}

void PreferencesWindowItem::onAppsWindowAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    updateSplitTunnelingAppsCount(apps);
    splitTunnelingAppsSearchWindowItem_->setApps(apps);
}

void PreferencesWindowItem::onNetworkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute> routes)
{
    splitTunnelingWindowItem_->setNetworkRoutesCount(routes.count());
}

void PreferencesWindowItem::onSearchModeExit()
{
    setPreferencesWindowToSplitTunnelingAppsHome();
}

void PreferencesWindowItem::onStAppsEscape()
{
    setPreferencesWindowToSplitTunnelingHome();
}

void PreferencesWindowItem::onStAppsSearchEscape()
{
    setPreferencesWindowToSplitTunnelingAppsHome();
}

void PreferencesWindowItem::onIpsAndHostnameEscape()
{
    setPreferencesWindowToSplitTunnelingHome();
}

void PreferencesWindowItem::onCurrentNetworkUpdated(ProtoTypes::NetworkInterface network)
{
    emit currentNetworkUpdated(network);
}

void PreferencesWindowItem::keyPressEvent(QKeyEvent *event)
{
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

    if (keyEvent->key() == Qt::Key_Escape && keyEvent->type() == QEvent::KeyPress)
    {
        if (isShowSubPage_)
        {
            moveOnePageBack();
        }
        else
        {
            emit escape();
        }
    }
}

void PreferencesWindowItem::setShowSubpageMode(bool isShowSubPage)
{
    isShowSubPage_ = isShowSubPage;
    tabControlItem_->setInSubpage(isShowSubPage);
}

QRectF PreferencesWindowItem::getBottomResizeArea()
{
    return QRectF(0, curHeight_ - BOTTOM_AREA_HEIGHT*G_SCALE, boundingRect().width(), BOTTOM_AREA_HEIGHT*G_SCALE );
}

void PreferencesWindowItem::updateChildItemsAfterHeightChanged()
{
    tabControlItem_->setHeight(curHeight_  - 99*G_SCALE);
    bottomResizeItem_->setPos(BOTTOM_RESIZE_ORIGIN_X * G_SCALE, curHeight_  - BOTTOM_RESIZE_OFFSET_Y*G_SCALE );
    scrollAreaItem_->setHeight(curHeight_  - 102*G_SCALE);
    advancedParametersWindowItem_->setHeight(curHeight_ / G_SCALE - 130); // only needed to extend the size of the parameters typing region
}

} // namespace PreferencesWindow
