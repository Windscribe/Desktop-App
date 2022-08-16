#ifndef TWOFACTORAUTHWINDOWITEM_H
#define TWOFACTORAUTHWINDOWITEM_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "../backend/backend.h"
#include "itwofactorauthwindow.h"
#include "twofactorauthokbutton.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "loginwindow/usernamepasswordentry.h"

namespace TwoFactorAuthWindow {

class TwoFactorAuthWindowItem : public ScalableGraphicsObject, public ITwoFactorAuthWindow
{
    Q_OBJECT
    Q_INTERFACES(ITwoFactorAuthWindow)
public:
    explicit TwoFactorAuthWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    QGraphicsObject *getGraphicsObject() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void resetState() override;
    void clearCurrentCredentials() override;
    void setCurrentCredentials(const QString &username, const QString &password) override;
    void setLoginMode(bool is_login_mode) override;
    void setErrorMessage(ERROR_MESSAGE_TYPE errorMessage) override;
    void setClickable(bool isClickable) override;
    void updateScaling() override;

signals:
    void addClick(const QString &code2fa) override;
    void loginClick(const QString &username, const QString &password,
                    const QString &code2fa) override;
    void escapeClick() override;
    void minimizeClick() override;
    void closeClick() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEscClicked();
    void onButtonClicked();

    void onTextOpacityChange(const QVariant &value);
    void onEscTextOpacityChange(const QVariant &value);

    void onCodeTextChanged(const QString &text);
    void onCodeEntryOpacityChanged(const QVariant &value);
    void onErrorChanged(const QVariant &value);
    void onDockedModeChanged(bool bIsDockedToTray);

private:
    void updatePositions();

    CommonGraphics::EscapeButton *escButton_;
    TwoFactorAuthOkButton *okButton_;

    IconButton *closeButton_;
    IconButton *minimizeButton_;

    LoginWindow::UsernamePasswordEntry *codeEntry_;

    double curEscTextOpacity_;
    double curTextOpacity_;
    double curCodeEntryOpacity_;
    QVariantAnimation codeEntryOpacityAnimation_;

    QString curErrorText_;
    double curErrorOpacity_;
    QVariantAnimation errorAnimation_;

    QString savedUsername_;
    QString savedPassword_;

    // constants:
    static constexpr int HEADER_HEIGHT = 71;
    static constexpr int CODE_ENTRY_POS_Y = 155;
    static constexpr int CODE_ENTRY_WIDTH = 244;
    static constexpr int OK_BUTTON_WIDTH = 78;
    static constexpr int OK_BUTTON_HEIGHT = 32;
    static constexpr int OK_BUTTON_POS_Y = 223;
};

}  // namespace TwoFactorAuthWindow

#endif // TWOFACTORAUTHWINDOWITEM_H
