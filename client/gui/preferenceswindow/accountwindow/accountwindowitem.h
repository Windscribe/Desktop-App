#ifndef ACCOUNTWINDOWITEM_H
#define ACCOUNTWINDOWITEM_H

#include "backend/preferences/accountinfo.h"
#include "commongraphics/basepage.h"
#include "commongraphics/bubblebuttondark.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "preferenceswindow/titleitem.h"
#include "accountdataitem.h"
#include "emailitem.h"
#include "planitem.h"

namespace PreferencesWindow {

class AccountWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit AccountWindowItem(ScalableGraphicsObject *parent, AccountInfo *accountInfo);

    QString caption();
    void setLoggedIn(bool loggedIn);
    void setConfirmEmailResult(bool bSuccess);

    void updateScaling() override;

private slots:
    void onUsernameChanged(const QString &username);
    void onEmailChanged(const QString &email);
    void onIsNeedConfirmEmailChanged(bool bNeedConfirm);
    void onPlanChanged(qint64 plan);
    void onExpireDateChanged(const QString &date);
    void onIsPremiumChanged(bool isPremium);
    void onTrafficUsedChanged(qint64 used);
    void onLastResetChanged(const QString &date);

    void onUpgradeClicked();

    void onLanguageChanged();

signals:
    void sendConfirmEmailClick();
    void accountLoginClick();
    void manageAccountClick();
    void addEmailButtonClick();

private:
    TitleItem *infoTitle_;
    AccountDataItem *usernameItem_;
    PreferenceGroup *infoGroup_;
    EmailItem *emailItem_;
    TitleItem *planTitle_;
    PreferenceGroup *planGroup_;
    PlanItem *planItem_;
    AccountDataItem *expireDateItem_;
    AccountDataItem *resetDateItem_;
    AccountDataItem *dataLeftItem_;
    PreferenceGroup *manageAccountGroup_;
    LinkItem *manageAccountItem_;

    QGraphicsTextItem *textItem_;
    CommonGraphics::BubbleButtonDark *loginButton_;
    void updateWidgetPos();

    qint64 plan_;
    qint64 trafficUsed_;

    bool isUnlimitedData() const;
    void setDataLeft() const;
    void updatePlanGroupItemVisibility();
};

} // namespace PreferencesWindow

#endif // ACCOUNTWINDOWITEM_H
