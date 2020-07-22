#include "emailitem.h"

#include <QPainter>
#include "GraphicResources/fontmanager.h"
#include "GraphicResources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

EmailItem::EmailItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50),
    isNeedConfirmEmail_(false), emailSendState_(EMAIL_NOT_INITIALIZED)
{
    dividerLine_ = new DividerLine(this, 276);
    dividerLine_->setPos(24, boundingRect().height() - dividerLine_->boundingRect().height());

    sendButton_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Sent!"),FontDescr(12, true),
                                                 QColor(3, 9, 28), true, this);
    connect(sendButton_, SIGNAL(clicked()), SIGNAL(sendEmailClick()));
    connect(sendButton_, SIGNAL(clicked()), SLOT(onSendEmailClick()));
    sendButton_->setCurrentOpacity(0.7);

    emptyEmailButton_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Add (Get 10GB/ mo)"),
                                                       FontDescr(12, true), QColor(255, 255, 255), true, this);
    connect(emptyEmailButton_, SIGNAL(clicked()), SIGNAL(emptyEmailButtonClick()));
    emptyEmailButton_->setPos(boundingRect().width() - emptyEmailButton_->boundingRect().width() - 16, (boundingRect().height() - emptyEmailButton_->boundingRect().height()) / 2 - 2);
    emptyEmailButton_->setClickable(false);
    emptyEmailButton_->hide();

    connect(this, SIGNAL(visibleChanged()), SLOT(onVisibleChanged()));

    setEmailSendState(EMAIL_NO_SEND);
    updateSendButtonPos();
}

void EmailItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 0)));
    qreal initialOpacity = painter->opacity();

    QFont *font = FontManager::instance().getFont(12, true);

    painter->setFont(*font);
    if (isNeedConfirmEmail_ && !email_.isEmpty())
    {
        QFontMetrics fm(*font);
        int textWidth = fm.width(tr("Email"));
        painter->setPen(QColor(253, 239, 0));
        QRectF rcText = boundingRect().adjusted(16*G_SCALE, 0, 0, -32*G_SCALE);
        painter->drawText(rcText, Qt::AlignVCenter, tr("Email"));

        IndependentPixmap *pixmapInfoIcon = ImageResourcesSvg::instance().getIndependentPixmap("preferences/INFO_YELLOW_ICON");
        int pixmapHeight = pixmapInfoIcon->height();
        pixmapInfoIcon->draw(16*G_SCALE + textWidth + 8*G_SCALE, static_cast<int>((rcText.height() - pixmapHeight) / 2), painter);

        painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, -16*G_SCALE, -32*G_SCALE), Qt::AlignVCenter | Qt::AlignRight, email_);

        QRectF rcYellow = boundingRect().adjusted(24*G_SCALE, 47*G_SCALE, 0, 0);
        painter->fillRect(QRectF(rcYellow.left(), rcYellow.top() - 1*G_SCALE, rcYellow.width(), 1*G_SCALE), QBrush(QColor(128, 124, 14)));
        painter->fillRect(rcYellow, QBrush(QColor(253, 239, 0)));
        painter->fillRect(QRectF(rcYellow.left(), rcYellow.bottom(), rcYellow.width(), 1*G_SCALE), QBrush(QColor(128, 124, 14)));

        painter->setFont(*FontManager::instance().getFont(12, true));
        painter->setPen(QColor(3, 9, 28));
        painter->setOpacity(0.7 * initialOpacity);
        painter->drawText(rcYellow.adjusted(8*G_SCALE, 0, 0, 0), Qt::AlignVCenter, tr("Please confirm your email"));

    }
    else
    {
        painter->setPen(QColor(255, 255, 255));
        painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, -2*G_SCALE), Qt::AlignVCenter, tr("Email"));

        if (!email_.isEmpty())
        {
            painter->setOpacity(0.5 * initialOpacity);
            painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, -16*G_SCALE, -2*G_SCALE), Qt::AlignVCenter | Qt::AlignRight, email_);
        }
    }
}

void EmailItem::setEmail(const QString &email)
{
    email_ = email;

    emptyEmailButton_->setClickable(email_.isEmpty());
    emptyEmailButton_->setVisible(email_.isEmpty());
    update();
    setNeedConfirmEmail(isNeedConfirmEmail_); //for update geometry
}

void EmailItem::setNeedConfirmEmail(bool b)
{
    isNeedConfirmEmail_ = b;
    if (isNeedConfirmEmail_ && !email_.isEmpty())
    {
        height_ = 80*G_SCALE;
        dividerLine_->hide();
        sendButton_->show();
    }
    else
    {
        height_ = 50*G_SCALE;
        dividerLine_->show();
        sendButton_->hide();
    }
    prepareGeometryChange();
    updateSendButtonPos();
    emit heightChanged(height_);
}


void EmailItem::setConfirmEmailResult(bool bSuccess)
{
    setEmailSendState(bSuccess ? EMAIL_SENT : EMAIL_FAILED_SEND);
}

void EmailItem::updateScaling()
{
    BaseItem::updateScaling();

    sendButton_->recalcBoundingRect();
    emptyEmailButton_->recalcBoundingRect();
    setNeedConfirmEmail(isNeedConfirmEmail_);   //for  update sizes
    dividerLine_->setPos(24*G_SCALE, boundingRect().height() - dividerLine_->boundingRect().height());
    emptyEmailButton_->setPos(boundingRect().width() - emptyEmailButton_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - emptyEmailButton_->boundingRect().height()) / 2 - 2*G_SCALE);

}

void EmailItem::onSendEmailClick()
{
    setEmailSendState(EMAIL_SENDING);
}

void EmailItem::onVisibleChanged()
{
    if (!isVisible() && (emailSendState_ == EMAIL_SENDING || emailSendState_ == EMAIL_SENT ||  emailSendState_ == EMAIL_FAILED_SEND))
    {
        setEmailSendState(EMAIL_NO_SEND);
    }
}

void EmailItem::updateSendButtonPos()
{
    QRectF rcYellow = boundingRect().adjusted(24*G_SCALE, 47*G_SCALE, 0, 0);
    sendButton_->setPos(rcYellow.width() - sendButton_->boundingRect().width() - 16*G_SCALE + rcYellow.left(), rcYellow.bottom() - sendButton_->boundingRect().height() - 7*G_SCALE);
}

void EmailItem::setEmailSendState(EmailItem::EMAIL_SEND_STATE state)
{
    if (state != emailSendState_)
    {
        if (state == EMAIL_NO_SEND)
        {
            sendButton_->setText(tr("Resend"));
            sendButton_->setCurrentOpacity(0.5);
            sendButton_->setClickable(true);
        }
        else if (state == EMAIL_SENDING)
        {
            sendButton_->setText(tr("Sending"));
            sendButton_->setCurrentOpacity(0.7);
            sendButton_->setClickable(false);
        }
        else if (state == EMAIL_SENT)
        {
            sendButton_->setText(tr("Sent!"));
            sendButton_->setCurrentOpacity(0.7);
            sendButton_->setClickable(false);
        }
        else if (state == EMAIL_FAILED_SEND)
        {
            sendButton_->setText(tr("Failed"));
            sendButton_->setCurrentOpacity(0.7);
            sendButton_->setClickable(false);
        }
        else
        {
            Q_ASSERT(false);
        }

        emailSendState_ = state;
        updateSendButtonPos();
    }
}


} // namespace PreferencesWindow
