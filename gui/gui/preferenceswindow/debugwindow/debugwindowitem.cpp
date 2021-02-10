#include "debugwindowitem.h"

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMessageBox>
#include "commongraphics/commongraphics.h"
#include "languagecontroller.h"
#include "utils/protoenumtostring.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

DebugWindowItem::DebugWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : BasePage(parent),
    preferences_(preferences), preferencesHelper_(preferencesHelper), currentScreen_(DEBUG_SCREEN_HOME)
  , pendingDebugSendResposeFromEngine_(false)

{
    setFlag(QGraphicsItem::ItemIsFocusable);

#ifdef Q_OS_WIN
    connect(preferences, SIGNAL(tapAdapterChanged(ProtoTypes::TapAdapterType)), SLOT(onTapAdapterPreferencesChanged(ProtoTypes::TapAdapterType)));
    connect(preferencesHelper, SIGNAL(ipv6StateInOSChanged(bool)), SLOT(onPreferencesIpv6InOSStateChanged(bool)));
#endif
    connect(preferences, SIGNAL(apiResolutionChanged(ProtoTypes::ApiResolution)), SLOT(onApiResolutionPreferencesChanged(ProtoTypes::ApiResolution)));
    connect(preferences, SIGNAL(dnsPolicyChanged(ProtoTypes::DnsPolicy)), SLOT(onDnsPolicyPreferencesChanged(ProtoTypes::DnsPolicy)));
    connect(preferences, SIGNAL(isIgnoreSslErrorsChanged(bool)), SLOT(onIgnoreSslErrorsPreferencesChanged(bool)));
    connect(preferences, SIGNAL(keepAliveChanged(bool)), SLOT(onKeepAlivePreferencesChanged(bool)));

    advParamtersItem_ = new SubPageItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::SubPageItem","Advanced Parameters"), true);
    connect(advParamtersItem_, SIGNAL(clicked()), SIGNAL(advParametersClick()));
    addItem(advParamtersItem_);

    viewLogItem_ = new ViewLogItem(this);
    connect(viewLogItem_, SIGNAL(viewButtonHoverEnter()), SLOT(onViewButtonHoverEnter()));
    connect(viewLogItem_, SIGNAL(sendButtonHoverEnter()), SLOT(onSendButtonHoverEnter()));
    connect(viewLogItem_, SIGNAL(buttonHoverLeave()), SLOT(onViewOrSendButtonHoverLeave()));
    connect(viewLogItem_, SIGNAL(viewLogClicked()), SIGNAL(viewLogClick()));
    connect(viewLogItem_, SIGNAL(sendLogClicked()), SLOT(onViewLogItemSendLogClicked()));
    addItem(viewLogItem_);

#ifdef Q_OS_WIN
    comboBoxTapAdapter_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "TAP Driver"), QString(), 50, Qt::transparent, 0, true);
    updateTapAdaptersList();
    comboBoxTapAdapter_->setCurrentItem((int)preferences_->tapAdapter());
    connect(comboBoxTapAdapter_, SIGNAL(currentItemChanged(QVariant)), SLOT(onTapAdapterChanged(QVariant)));
    addItem(comboBoxTapAdapter_);

    checkBoxIPv6_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "IPv6"), "");
    checkBoxIPv6_->setState(preferencesHelper->getIpv6StateInOS());
    connect(checkBoxIPv6_, SIGNAL(stateChanged(bool)), SLOT(onIPv6StateChanged(bool)));
    addItem(checkBoxIPv6_);
#endif

    apiResolutionItem_ = new ApiResolutionItem(this);
    apiResolutionItem_->setApiResolution(preferences->apiResolution());
    connect(apiResolutionItem_, SIGNAL(apiResolutionChanged(ProtoTypes::ApiResolution)), SLOT(onApiResolutionChanged(ProtoTypes::ApiResolution)));
    addItem(apiResolutionItem_);

    cbIgnoreSslErrors_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Ignore SSL Errors"), "");
    cbIgnoreSslErrors_->setState(preferences->isIgnoreSslErrors());
    connect(cbIgnoreSslErrors_, SIGNAL(stateChanged(bool)), SLOT(onIgnoreSslErrorsStateChanged(bool)));
    addItem(cbIgnoreSslErrors_);

    cbKeepAlive_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Client-side Keepalive"), "");
    cbKeepAlive_->setState(preferences->keepAlive());
    connect(cbKeepAlive_, SIGNAL(stateChanged(bool)), SLOT(onKeepAliveStateChanged(bool)));
    addItem(cbKeepAlive_);

    comboBoxAppInternalDns_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "App Internal DNS"), QString(), 50, Qt::transparent, 0, true);
    QList< QPair<QString, int> > dnsTypes = ProtoEnumToString::instance().getEnums(ProtoTypes::DnsPolicy_descriptor());
    Q_FOREACH(auto d, dnsTypes)
    {
        comboBoxAppInternalDns_->addItem(d.first, d.second);
    }

    comboBoxAppInternalDns_->setCurrentItem(preferences->dnsPolicy());
    connect(comboBoxAppInternalDns_, SIGNAL(currentItemChanged(QVariant)), SLOT(onAppInternalDnsItemChanged(QVariant)));
    addItem(comboBoxAppInternalDns_);
}

QString DebugWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Debug");
}

DEBUG_SCREEN DebugWindowItem::getScreen()
{
    return currentScreen_;
}

void DebugWindowItem::setScreen(DEBUG_SCREEN screen)
{
    currentScreen_ = screen;
}

void DebugWindowItem::setDebugLogResult(bool success)
{
    pendingDebugSendResposeFromEngine_ = false;

    QString displayString = tr("Log failed to send");
    if (success)
    {
        displayString = tr("Log sent!");
    }

    showSendButtonToolTip(displayString);

    QTimer::singleShot(TOOLTIP_TIMEOUT, [](){
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOG_SENT);
    });
}

void DebugWindowItem::updateScaling()
{
    BasePage::updateScaling();
    apiResolutionItem_->updateScaling();
}

#ifdef Q_OS_WIN
void DebugWindowItem::onTapAdapterChanged(QVariant v)
{
    /*ProtoTypes::TapAdapterType tapAdapter = (ProtoTypes::TapAdapterType)v.toInt();
    if (tapAdapter == ProtoTypes::TAP_ADAPTER_5 || tapAdapter == ProtoTypes::TAP_ADAPTER_6)
    {
        comboBoxTapAdapter_->removeItem(ProtoTypes::TAP_ADAPTER_NOT_INSTALLED);
        emit installTapAdapter(tapAdapter);
    }
    preferencesHelper_->setInstalledTapAdapter(tapAdapter);*/
    preferences_->setTapAdapter((ProtoTypes::TapAdapterType)v.toInt());
}

void DebugWindowItem::onIPv6StateChanged(bool isChecked)
{
    QMessageBox msgBox(g_mainWindow);
    msgBox.setText(tr("In order to disable IPv6, a computer restart is required. Do it now?"));
    QAbstractButton* pButtonYes = msgBox.addButton(tr("Yes"), QMessageBox::NoRole);
    QAbstractButton* pButtonLater = msgBox.addButton(tr("Restart later"), QMessageBox::NoRole);
    QAbstractButton* pButtonCancel = msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    msgBox.exec();

    if (msgBox.clickedButton() == pButtonYes)
    {
        emit setIpv6StateInOS(isChecked, true);
    }
    else if (msgBox.clickedButton() == pButtonLater)
    {
        emit setIpv6StateInOS(isChecked, false);
    }
    else if (msgBox.clickedButton() == pButtonCancel)
    {
        checkBoxIPv6_->setState(!isChecked);
    }
}
#endif

void DebugWindowItem::onApiResolutionChanged(const ProtoTypes::ApiResolution &ar)
{
    preferences_->setApiResolution(ar);

    if (!ar.is_automatic()) // only scroll when opening
    {
        // 93 is expanded height
        emit scrollToPosition(static_cast<int>(apiResolutionItem_->y()) + 93 );
    }
}

void DebugWindowItem::onIgnoreSslErrorsStateChanged(bool isChecked)
{
    preferences_->setIgnoreSslErrors(isChecked);
}

void DebugWindowItem::onKeepAliveStateChanged(bool isChecked)
{
    preferences_->setKeepAlive(isChecked);
}

void DebugWindowItem::onAppInternalDnsItemChanged(QVariant dns)
{
    preferences_->setDnsPolicy((ProtoTypes::DnsPolicy)dns.toInt());
}

void DebugWindowItem::onViewLogItemSendLogClicked()
{
    // delay so that the hoverLeave->cancel doesn't hide this tooltip
    QTimer::singleShot(50,  [this](){
        if (pendingDebugSendResposeFromEngine_)
        {
            const QString &text = "Sending...";
            showSendButtonToolTip(text);
        }
    });

    pendingDebugSendResposeFromEngine_ = true;
    emit sendLogClick();
}

void DebugWindowItem::onViewButtonHoverEnter()
{
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(viewLogItem_->scenePos()));

    int posX = globalPt.x() + 225*G_SCALE;
    int posY = globalPt.y() + 12*G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_VIEW_LOG);
    ti.x = posX;
    ti.y = posY;
    ti.title = tr("View Log");
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.1;
    TooltipController::instance().showTooltipBasic(ti);
}

void DebugWindowItem::onSendButtonHoverEnter()
{
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(viewLogItem_->scenePos()));

    int posX = globalPt.x() + 255*G_SCALE;
    int posY = globalPt.y() +  12*G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_SEND_LOG);
    ti.x = posX;
    ti.y = posY;
    ti.title = tr("Send Log");
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.1;
    TooltipController::instance().showTooltipBasic(ti);
}

void DebugWindowItem::onViewOrSendButtonHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_SEND_LOG);
    TooltipController::instance().hideTooltip(TOOLTIP_ID_VIEW_LOG);
}

void DebugWindowItem::onIgnoreSslErrorsPreferencesChanged(bool b)
{
    cbIgnoreSslErrors_->setState(b);
}

void DebugWindowItem::onKeepAlivePreferencesChanged(bool b)
{
    cbKeepAlive_->setState(b);
}

void DebugWindowItem::onDnsPolicyPreferencesChanged(ProtoTypes::DnsPolicy d)
{
    comboBoxAppInternalDns_->setCurrentItem((int)d);
}

void DebugWindowItem::onApiResolutionPreferencesChanged(const ProtoTypes::ApiResolution &ar)
{
    apiResolutionItem_->setApiResolution(ar);
}

#ifdef Q_OS_WIN
void DebugWindowItem::onPreferencesIpv6InOSStateChanged(bool bEnabled)
{
    checkBoxIPv6_->setState(bEnabled);
}

void DebugWindowItem::onTapAdapterPreferencesChanged(ProtoTypes::TapAdapterType tapAdapter)
{
    comboBoxTapAdapter_->setCurrentItem((int)tapAdapter);
}

void DebugWindowItem::updateTapAdaptersList()
{
    comboBoxTapAdapter_->clear();
    comboBoxTapAdapter_->addItem(ProtoEnumToString::instance().toString(ProtoTypes::TAP_ADAPTER), (int)ProtoTypes::TAP_ADAPTER);

    //if (comboBoxOpenVpn_->hasItems())
    {
        //if (comboBoxOpenVpn_->currentItem().toString().contains("2.5"))
        {
            comboBoxTapAdapter_->addItem(ProtoEnumToString::instance().toString(ProtoTypes::WINTUN_ADAPTER), (int)ProtoTypes::WINTUN_ADAPTER);
        }
    }
    //comboBoxTapAdapter_->setCurrentItem((int)ProtoTypes::TAP_ADAPTER);
}
#endif

void DebugWindowItem::onLanguageChanged()
{
    /*QVariant dnsWhileDisconnectedSelected = comboBoxDNSWhileDisconnected_->currentItem();

    comboBoxDNSWhileDisconnected_->clear();

    QList<DNSWhileDisconnectedType> allDNSTypes = DNSWhileDisconnectedType::allAvailableTypes();
    Q_FOREACH(const DNSWhileDisconnectedType &d, allDNSTypes)
    {
        comboBoxDNSWhileDisconnected_->addItem(d.toString(), static_cast<int>(d.type()));
    }
    comboBoxDNSWhileDisconnected_->setCurrentItem(dnsWhileDisconnectedSelected.toInt());*/
}

void DebugWindowItem::hideOpenPopups()
{
    BasePage::hideOpenPopups();

#ifdef Q_OS_WIN
    comboBoxTapAdapter_->hideMenu();
#endif
}

void DebugWindowItem::showSendButtonToolTip(const QString &text)
{
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(viewLogItem_->scenePos()));

    int posX = globalPt.x() + 255 * G_SCALE;
    int posY = globalPt.y() + 12 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOG_SENT);
    ti.x = posX;
    ti.y = posY;
    ti.title = text;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.1;
    ti.delay = 0;
    TooltipController::instance().showTooltipBasic(ti);
}

} // namespace PreferencesWindow
