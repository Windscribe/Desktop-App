#include "emailitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/ws_assert.h"

namespace PreferencesWindow {

EmailItem::EmailItem(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), isNeedConfirmEmail_(false), msgHeight_(0),
    emailSendState_(EMAIL_NOT_INITIALIZED)
{
    sendButton_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Sent!"),
                                                 FontDescr(10, true),
                                                 QColor(253, 239, 0),
                                                 true,
                                                 this);
    sendButton_->setTextAlignment(Qt::AlignLeft);
    sendButton_->setUnhoverOpacity(OPACITY_FULL);
    sendButton_->setCurrentOpacity(OPACITY_FULL);
    connect(sendButton_, &CommonGraphics::TextButton::clicked, this, &EmailItem::sendEmailClick);
    connect(sendButton_, &CommonGraphics::TextButton::clicked, this, &EmailItem::onSendEmailClick);

    emptyEmailButton_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Add"),
                                                       FontDescr(12, true),
                                                       Qt::white,
                                                       true,
                                                       this);
    connect(emptyEmailButton_, &CommonGraphics::TextButton::clicked, this, &EmailItem::emptyEmailButtonClick);
    emptyEmailButton_->setClickable(false);
    emptyEmailButton_->hide();

    connect(this, &EmailItem::visibleChanged, this, &EmailItem::onVisibleChanged);

    setEmailSendState(EMAIL_NO_SEND);
    updatePositions();
}

void EmailItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont *font = FontManager::instance().getFont(12, true);
    QFontMetrics fm(*font);
    int textWidth = fm.horizontalAdvance(tr(EMAIL_TEXT));

    painter->setFont(*font);
    if (email_.isEmpty() || isNeedConfirmEmail_)
    {
        QColor color;

        QSharedPointer<IndependentPixmap> pixmapInfoIcon;
        if (email_.isEmpty())
        {
            color = Qt::white;
            painter->setPen(color);
            pixmapInfoIcon = ImageResourcesSvg::instance().getIndependentPixmap("preferences/INFO_WHITE_ICON");
        }
        else
        {
            pixmapInfoIcon = ImageResourcesSvg::instance().getIndependentPixmap("preferences/INFO_YELLOW_ICON");
            color = QColor(253, 239, 0);
            painter->setPen(color);

            painter->setOpacity(OPACITY_HALF);
            QFont *font = FontManager::instance().getFont(12, false);
            painter->setFont(*font);
            QFontMetrics fmEmail(*font);
            QString elidedEmail = fmEmail.elidedText(email_, Qt::TextElideMode::ElideMiddle, boundingRect().width() - (2*PREFERENCES_MARGIN + 2*APP_ICON_MARGIN_X + ICON_WIDTH)*G_SCALE - textWidth);
            painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE), Qt::AlignRight, elidedEmail);
        }
        painter->setOpacity(OPACITY_FULL);
        pixmapInfoIcon->draw(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);

        painter->setFont(*FontManager::instance().getFont(12, true));
        painter->drawText(boundingRect().adjusted((PREFERENCES_MARGIN + ICON_WIDTH + DESCRIPTION_MARGIN)*G_SCALE, PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE), Qt::AlignLeft, tr(EMAIL_TEXT));

        // subrect
        painter->setOpacity(OPACITY_WARNING_BACKGROUND);
        painter->setPen(Qt::NoPen);
        QPainterPath path;
        QRectF rect = boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                              WARNING_OFFSET_Y*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE,
                                              -PREFERENCES_MARGIN*G_SCALE);
        path.addRoundedRect(rect, SMALL_ROUNDED_RECT_RADIUS*G_SCALE, SMALL_ROUNDED_RECT_RADIUS*G_SCALE);
        painter->fillPath(path, QBrush(color));
        painter->setPen(Qt::SolidLine);

        // Text inside subrect
        painter->setFont(*FontManager::instance().getFont(10, false));
        QString text;
        if (email_.isEmpty())
        {
            painter->setPen(Qt::white);
            painter->setOpacity(OPACITY_HALF);
            text = tr(ADD_TEXT);
        }
        else
        {
            painter->setPen(QColor(253, 239, 0));
            painter->setOpacity(OPACITY_FULL);
            text = tr(CONFIRM_TEXT);
        }
        painter->drawText(rect.adjusted(DESCRIPTION_MARGIN*G_SCALE, DESCRIPTION_MARGIN*G_SCALE, -DESCRIPTION_MARGIN*G_SCALE, -DESCRIPTION_MARGIN*G_SCALE),
                          Qt::AlignLeft | Qt::TextWordWrap,
                          text);
    }
    else
    {
        painter->setPen(Qt::white);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE), Qt::AlignLeft, tr(EMAIL_TEXT));

        if (!email_.isEmpty())
        {
            painter->setOpacity(OPACITY_HALF);
            QFont *font = FontManager::instance().getFont(12, false);
            painter->setFont(*font);
            QFontMetrics fmEmail(*font);
            QString elidedEmail = fmEmail.elidedText(email_, Qt::TextElideMode::ElideMiddle, boundingRect().width() - (2*PREFERENCES_MARGIN + APP_ICON_MARGIN_X)*G_SCALE - textWidth);
            painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE), Qt::AlignRight, elidedEmail);
        }
    }
}

void EmailItem::setEmail(const QString &email)
{
    email_ = email;
    updatePositions();
}

void EmailItem::setNeedConfirmEmail(bool b)
{
    isNeedConfirmEmail_ = b;
    updatePositions();
}

void EmailItem::updatePositions()
{
    if (email_.isEmpty())
    {
        emptyEmailButton_->setClickable(true);
        emptyEmailButton_->setVisible(true);
        sendButton_->hide();

        QFontMetrics fm(*FontManager::instance().getFont(10, false));
        msgHeight_ = fm.boundingRect(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                                             0,
                                                             -PREFERENCES_MARGIN*G_SCALE,
                                                             0).toRect(),
                                     Qt::AlignLeft | Qt::TextWordWrap,
                                     tr(ADD_TEXT)).height();

        height_ = (WARNING_OFFSET_Y + 2*DESCRIPTION_MARGIN + PREFERENCES_MARGIN)*G_SCALE + msgHeight_;
        emptyEmailButton_->setPos(boundingRect().width() - emptyEmailButton_->boundingRect().width() - PREFERENCES_MARGIN*G_SCALE,
                                  PREFERENCES_MARGIN*G_SCALE);

    }
    else if (isNeedConfirmEmail_)
    {
        emptyEmailButton_->setClickable(false);
        emptyEmailButton_->setVisible(false);

        QFontMetrics fm(*FontManager::instance().getFont(10, false));
        msgHeight_ = fm.boundingRect(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                                             PREFERENCES_MARGIN*G_SCALE,
                                                             -PREFERENCES_MARGIN*G_SCALE + sendButton_->boundingRect().width(),
                                                             0).toRect(),
                                     Qt::AlignLeft | Qt::TextWordWrap,
                                     tr(CONFIRM_TEXT)).height();

        height_ = (WARNING_OFFSET_Y + 2*DESCRIPTION_MARGIN + PREFERENCES_MARGIN)*G_SCALE + msgHeight_;
        sendButton_->setPos(boundingRect().width() - sendButton_->boundingRect().width() - 2*PREFERENCES_MARGIN*G_SCALE,
                            (WARNING_OFFSET_Y + DESCRIPTION_MARGIN)*G_SCALE);
        sendButton_->show();
    }
    else
    {
        emptyEmailButton_->setClickable(false);
        emptyEmailButton_->setVisible(false);
        sendButton_->hide();

        height_ = PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE;
    }
    prepareGeometryChange();
    emit heightChanged(height_);
    update();
}

void EmailItem::setConfirmEmailResult(bool bSuccess)
{
    setEmailSendState(bSuccess ? EMAIL_SENT : EMAIL_FAILED_SEND);
}

void EmailItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
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

void EmailItem::setEmailSendState(EmailItem::EMAIL_SEND_STATE state)
{
    if (state != emailSendState_)
    {
        if (state == EMAIL_NO_SEND)
        {
            sendButton_->setText(tr("Resend"));
            sendButton_->setClickable(true);
        }
        else if (state == EMAIL_SENDING)
        {
            sendButton_->setText(tr("Sending"));
            sendButton_->setClickable(false);
        }
        else if (state == EMAIL_SENT)
        {
            sendButton_->setText(tr("Sent!"));
            sendButton_->setClickable(false);
        }
        else if (state == EMAIL_FAILED_SEND)
        {
            sendButton_->setText(tr("Failed"));
            sendButton_->setClickable(false);
        }
        else
        {
            WS_ASSERT(false);
        }

        emailSendState_ = state;
        updatePositions();
    }
}

} // namespace PreferencesWindow
