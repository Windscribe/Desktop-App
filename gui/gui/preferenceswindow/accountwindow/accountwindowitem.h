#ifndef ACCOUNTWINDOWITEM_H
#define ACCOUNTWINDOWITEM_H

#include "../backend/preferences/accountinfo.h"
#include "../basepage.h"
#include "usernameitem.h"
#include "emailitem.h"
#include "planitem.h"
#include "expiredateitem.h"
#include "commongraphics/bubblebuttondark.h"
#include "../openurlitem.h"

namespace PreferencesWindow {

class AccountWindowItem : public BasePage
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
    void onAuthHashChanged(const QString &authHash);
    void onIsPremiumChanged(bool isPremium);

    void onUpgradeClicked();

    void onLanguageChanged();

signals:
    void sendConfirmEmailClick();
    void noAccountLoginClick();
    void editAccountDetailsClick();
    void addEmailButtonClick();

private:
    UsernameItem *usernameItem_;
    EmailItem *emailItem_;
    PlanItem *planItem_;
    ExpireDateItem *expireDateItem_;
    OpenUrlItem *editAccountItem_;
    QString authHash_;

    QGraphicsTextItem *textItem_;
    CommonGraphics::BubbleButtonDark *loginButton_;
    void updateWidgetPos();
};

} // namespace PreferencesWindow

#endif // ACCOUNTWINDOWITEM_H
