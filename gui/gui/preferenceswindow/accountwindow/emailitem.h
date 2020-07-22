#ifndef EMAILITEM_H
#define EMAILITEM_H

#include "../baseitem.h"
#include "../dividerline.h"
#include "CommonGraphics/textbutton.h"


namespace PreferencesWindow {

class EmailItem : public BaseItem
{
    Q_OBJECT
public:
    explicit EmailItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setEmail(const QString &email);
    void setNeedConfirmEmail(bool b);
    void setConfirmEmailResult(bool bSuccess);

    virtual void updateScaling();

signals:
    void sendEmailClick();
    void emptyEmailButtonClick();

private slots:
    void onSendEmailClick();
    void onVisibleChanged();

private:
    QString email_;
    bool isNeedConfirmEmail_;

    enum EMAIL_SEND_STATE { EMAIL_NOT_INITIALIZED, EMAIL_NO_SEND, EMAIL_SENDING, EMAIL_SENT, EMAIL_FAILED_SEND };
    EMAIL_SEND_STATE emailSendState_;

    DividerLine *dividerLine_;
    CommonGraphics::TextButton *sendButton_;
    CommonGraphics::TextButton *emptyEmailButton_;

    void updateSendButtonPos();
    void setEmailSendState(EMAIL_SEND_STATE state);

};

} // namespace PreferencesWindow

#endif // EMAILITEM_H
