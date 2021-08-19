#ifndef EMAILITEM_H
#define EMAILITEM_H

#include "../baseitem.h"
#include "../dividerline.h"
#include "commongraphics/textbutton.h"


namespace PreferencesWindow {

class EmailItem : public BaseItem
{
    Q_OBJECT
public:
    explicit EmailItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setEmail(const QString &email);
    void setNeedConfirmEmail(bool b);
    void setConfirmEmailResult(bool bSuccess);

    void updateScaling() override;

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
