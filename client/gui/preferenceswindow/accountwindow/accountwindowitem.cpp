#include "accountwindowitem.h"

#include <QPainter>
#include <QDesktopServices>
#include <QUrl>
#include <QTextDocument>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"
#include "utils/utils.h"

namespace PreferencesWindow {

AccountWindowItem::AccountWindowItem(ScalableGraphicsObject *parent, AccountInfo *accountInfo)
  : CommonGraphics::BasePage(parent), plan_(accountInfo->plan()), trafficUsed_(accountInfo->trafficUsed())
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    connect(accountInfo, &AccountInfo::usernameChanged, this, &AccountWindowItem::onUsernameChanged);
    connect(accountInfo, &AccountInfo::emailChanged, this, &AccountWindowItem::onEmailChanged);
    connect(accountInfo, &AccountInfo::isNeedConfirmEmailChanged, this, &AccountWindowItem::onIsNeedConfirmEmailChanged);
    connect(accountInfo, &AccountInfo::planChanged, this, &AccountWindowItem::onPlanChanged);
    connect(accountInfo, &AccountInfo::expireDateChanged, this, &AccountWindowItem::onExpireDateChanged);
    connect(accountInfo, &AccountInfo::authHashChanged, this, &AccountWindowItem::onAuthHashChanged);
    connect(accountInfo, &AccountInfo::isPremiumChanged, this, &AccountWindowItem::onIsPremiumChanged);
    connect(accountInfo, &AccountInfo::trafficUsedChanged, this, &AccountWindowItem::onTrafficUsedChanged);
    connect(accountInfo, &AccountInfo::lastResetChanged, this, &AccountWindowItem::onLastResetChanged);

    infoTitle_ = new TitleItem(this, tr("INFO"));
    addItem(infoTitle_);

    infoGroup_ = new PreferenceGroup(this);
    usernameItem_ = new AccountDataItem(infoGroup_, tr("Username"), accountInfo->username());
    infoGroup_->addItem(usernameItem_);

    emailItem_ = new EmailItem(infoGroup_);
    connect(emailItem_, &EmailItem::emptyEmailButtonClick, this, &AccountWindowItem::addEmailButtonClick);
    connect(emailItem_, &EmailItem::sendEmailClick, this, &AccountWindowItem::sendConfirmEmailClick);
    emailItem_->setEmail(accountInfo->email());
    emailItem_->setNeedConfirmEmail(accountInfo->isNeedConfirmEmail());
    infoGroup_->addItem(emailItem_);
    addItem(infoGroup_);

    planTitle_ = new TitleItem(this, tr("PLAN"));
    addItem(planTitle_);

    planGroup_ = new PreferenceGroup(this);
    planItem_ = new PlanItem(planGroup_);
    connect(planItem_, &PlanItem::upgradeClicked, this, &AccountWindowItem::onUpgradeClicked);
    planItem_->setPlan(accountInfo->plan());
    onIsPremiumChanged(accountInfo->isPremium());
    planGroup_->addItem(planItem_);

    expireDateItem_ = new AccountDataItem(planGroup_, tr("Expires On"), accountInfo->expireDate());
    planGroup_->addItem(expireDateItem_);

    resetDateItem_ = new AccountDataItem(planGroup_,
                                         tr("Reset Date"),
                                         QDate::fromString(accountInfo->lastReset(), "yyyy-MM-dd").addMonths(1).toString("yyyy-MM-dd"));
    planGroup_->addItem(resetDateItem_);
    dataLeftItem_ = new AccountDataItem(planGroup_,
                                        tr("Data Left"),
                                        Utils::humanReadableByteCount(accountInfo->plan() - accountInfo->trafficUsed(), false, true));
    planGroup_->addItem(dataLeftItem_);
    addItem(planGroup_);

    authHash_ = accountInfo->authHash();

    // don't set url on this since we need to first grab the temp_session_token from the api first
    // this is because we want to obscure user auth info by POST request to API rather than QDesktopServices HTTP GET
    // for the POST we use the ServerAPI in the backend and we get a temporary one-time token back from the API that
    // can be safetly used in a GET command 
    manageAccountGroup_ = new PreferenceGroup(this);
    manageAccountItem_ = new LinkItem(manageAccountGroup_, LinkItem::LinkType::EXTERNAL_LINK, tr("Manage Account"));
    connect(manageAccountItem_, &LinkItem::clicked, this, &AccountWindowItem::manageAccountClick);
    manageAccountGroup_->addItem(manageAccountItem_);
    addItem(manageAccountGroup_);

    // Below items for case when not logged in
    textItem_ = new QGraphicsTextItem(this);
    textItem_->setPlainText(tr("Login to view your account info"));
    textItem_->setFont(*FontManager::instance().getFont(14, false));
    textItem_->setDefaultTextColor(Qt::white);
    textItem_->setTextWidth(125);
    textItem_->document()->setDefaultTextOption(QTextOption(Qt::AlignHCenter));

    loginButton_ = new CommonGraphics::BubbleButtonDark(this, 69, 24, 12, 20);
    loginButton_->setText(tr("Login"));
    loginButton_->setFont(FontDescr(12,false));
    connect(loginButton_, &CommonGraphics::BubbleButtonDark::clicked, this, &AccountWindowItem::accountLoginClick);

    updateWidgetPos();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &AccountWindowItem::onLanguageChanged);
}

QString AccountWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Account");
}

void AccountWindowItem::setLoggedIn(bool loggedIn)
{
    infoTitle_->setVisible(loggedIn);
    infoGroup_->setVisible(loggedIn);
    planTitle_->setVisible(loggedIn);
    planGroup_->setVisible(loggedIn);
    // update visibility for subitems
    onIsPremiumChanged(planItem_->isPremium());
    manageAccountGroup_->setVisible(loggedIn);

    textItem_->setVisible(!loggedIn);
    loginButton_->setVisible(!loggedIn);
}

void AccountWindowItem::setConfirmEmailResult(bool bSuccess)
{
    emailItem_->setConfirmEmailResult(bSuccess);
}

void AccountWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
    textItem_->setFont(*FontManager::instance().getFont(14, false));
    textItem_->setTextWidth(125*G_SCALE);
    updateWidgetPos();
}

void AccountWindowItem::onUsernameChanged(const QString &username)
{
    usernameItem_->setValue2(username);
}

void AccountWindowItem::onEmailChanged(const QString &email)
{
    emailItem_->setEmail(email);
}

void AccountWindowItem::onIsNeedConfirmEmailChanged(bool bNeedConfirm)
{
    emailItem_->setNeedConfirmEmail(bNeedConfirm);
}

void AccountWindowItem::onPlanChanged(qint64 plan)
{
    plan_ = plan;
    planItem_->setPlan(plan);
    dataLeftItem_->setValue2(Utils::humanReadableByteCount(qMax(0, plan_ - trafficUsed_), false, true));
}

void AccountWindowItem::onExpireDateChanged(const QString &date)
{
    expireDateItem_->setValue2(date);
}

void AccountWindowItem::onLastResetChanged(const QString &date)
{
    resetDateItem_->setValue2(QDate::fromString(date, "yyyy-MM-dd").addMonths(1).toString("yyyy-MM-dd"));
}

void AccountWindowItem::onTrafficUsedChanged(qint64 used)
{
    trafficUsed_ = used;
    dataLeftItem_->setValue2(Utils::humanReadableByteCount(qMax(0, plan_ - trafficUsed_), false, true));
}

void AccountWindowItem::onAuthHashChanged(const QString &authHash)
{
    authHash_ = authHash;
}

void AccountWindowItem::onUpgradeClicked()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/upgrade?pcpid=desktop_upgrade").arg(HardcodedSettings::instance().serverUrl())));
}

void AccountWindowItem::onLanguageChanged()
{
    loginButton_->setText(tr("Login"));
    textItem_->setPlainText(tr("Login to view your account info"));

    updateWidgetPos();
}

void AccountWindowItem::updateWidgetPos()
{
    int textCenterX = static_cast<int>(boundingRect().width()/2 - textItem_->boundingRect().width()/2);
    textItem_->setPos(textCenterX, 85 * G_SCALE);

    int loginCenterX = static_cast<int>(boundingRect().width()/2 - loginButton_->boundingRect().width()/2);
    loginButton_->setPos(loginCenterX, 155 * G_SCALE);
}

void AccountWindowItem::onIsPremiumChanged(bool isPremium)
{
    planItem_->setIsPremium(isPremium);
    if (isPremium)
    {
        planGroup_->showItems(planGroup_->indexOf(expireDateItem_));
        planGroup_->hideItems(planGroup_->indexOf(resetDateItem_), planGroup_->indexOf(dataLeftItem_));
    }
    else
    {
        planGroup_->hideItems(planGroup_->indexOf(expireDateItem_));
        planGroup_->showItems(planGroup_->indexOf(resetDateItem_), planGroup_->indexOf(dataLeftItem_));
    }
}

} // namespace PreferencesWindow
