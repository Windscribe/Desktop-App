#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "backend/backend.h"
#include "twofactorauthokbutton.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "loginwindow/usernamepasswordentry.h"

namespace TwoFactorAuthWindow {

class TwoFactorAuthWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    enum ERROR_MESSAGE_TYPE { ERR_MSG_EMPTY, ERR_MSG_NO_CODE, ERR_MSG_INVALID_CODE };

    explicit TwoFactorAuthWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;
    void updateScaling() override;

    void resetState();
    void clearCurrentCredentials();
    void setCurrentCredentials(const QString &username, const QString &password);
    void setLoginMode(bool is_login_mode);
    void setErrorMessage(ERROR_MESSAGE_TYPE errorMessage);
    void setClickable(bool isClickable);

signals:
    void addClick(const QString &code2fa);
    void loginClick(const QString &username, const QString &password, const QString &code2fa);
    void escapeClick();
    void minimizeClick();
    void closeClick();

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
    static constexpr int OK_BUTTON_POS_Y = 223;
};

}  // namespace TwoFactorAuthWindow
