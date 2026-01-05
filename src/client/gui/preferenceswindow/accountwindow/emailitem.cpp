#include "emailitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/ws_assert.h"

namespace PreferencesWindow {

EmailItem::EmailItem(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), isNeedConfirmEmail_(false), spinnerRotation_(0), msgHeight_(0),
    emailSendState_(EMAIL_NOT_INITIALIZED)
{
    sendButton_ = new CommonGraphics::TextButton(tr("Sent!"),
                                                 FontDescr(12, QFont::Bold),
                                                 QColor(253, 239, 0),
                                                 true,
                                                 this);
    sendButton_->setTextAlignment(Qt::AlignLeft);
    sendButton_->setUnhoverOpacity(OPACITY_FULL);
    sendButton_->setCurrentOpacity(OPACITY_FULL);
    sendButton_->setMarginHeight(0);
    connect(sendButton_, &CommonGraphics::TextButton::clicked, this, &EmailItem::sendEmailClick);
    connect(sendButton_, &CommonGraphics::TextButton::clicked, this, &EmailItem::onSendEmailClick);

    emptyEmailButton_ = new CommonGraphics::TextButton(tr(ADD_TEXT),
                                                       FontDescr(14, QFont::Bold),
                                                       Qt::white,
                                                       true,
                                                       this);
    connect(emptyEmailButton_, &CommonGraphics::TextButton::clicked, this, &EmailItem::onEmptyEmailButtonClick);
    emptyEmailButton_->setClickable(false);
    emptyEmailButton_->setMarginHeight(0);
    emptyEmailButton_->hide();
    connect(this, &EmailItem::visibleChanged, this, &EmailItem::onVisibleChanged);

    connect(&spinnerAnimation_, &QVariantAnimation::valueChanged, this, &EmailItem::onSpinnerRotationChanged);
    connect(&spinnerAnimation_, &QVariantAnimation::finished, this, &EmailItem::onSpinnerRotationFinished);

    setEmailSendState(EMAIL_NO_SEND);
    updatePositions();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &EmailItem::onLanguageChanged);
}

void EmailItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(14, QFont::DemiBold);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(tr(EMAIL_TEXT));

    painter->setFont(font);
    if (email_.isEmpty() || isNeedConfirmEmail_) {
        QColor color;

        QSharedPointer<IndependentPixmap> pixmapInfoIcon;
        if (email_.isEmpty()) {
            color = Qt::white;
            painter->setPen(color);
            pixmapInfoIcon = ImageResourcesSvg::instance().getIndependentPixmap("preferences/INFO_WHITE_ICON");
        } else {
            pixmapInfoIcon = ImageResourcesSvg::instance().getIndependentPixmap("preferences/INFO_YELLOW_ICON");
            color = QColor(253, 239, 0);
            painter->setPen(color);

            painter->setOpacity(OPACITY_SIXTY);
            QFont font = FontManager::instance().getFont(14,  QFont::Normal);
            painter->setFont(font);
            QFontMetrics fmEmail(font);
            QString elidedEmail = fmEmail.elidedText(email_, Qt::TextElideMode::ElideMiddle, boundingRect().width() - (2*PREFERENCES_MARGIN_X + 2*APP_ICON_MARGIN_X + ICON_WIDTH)*G_SCALE - textWidth);
            painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, -(boundingRect().height() - PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)), Qt::AlignRight | Qt::AlignVCenter, elidedEmail);
        }
        painter->setOpacity(OPACITY_FULL);
        pixmapInfoIcon->draw(PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);

        if (emailSendState_ == EMAIL_ADDING) {
            QSharedPointer<IndependentPixmap> p;
            painter->setOpacity(1.0);
            p = ImageResourcesSvg::instance().getIndependentPixmap("SPINNER");
            painter->save();
            painter->translate(static_cast<int>(boundingRect().width() - p->width()/2 - PREFERENCES_MARGIN_X*G_SCALE), PREFERENCES_ITEM_Y*G_SCALE + p->height()/2);
            painter->rotate(spinnerRotation_);
            p->draw(-p->width()/2, -p->height()/2, painter);
            painter->restore();
        }

        painter->setFont(FontManager::instance().getFont(14, QFont::DemiBold));
        painter->drawText(boundingRect().adjusted((PREFERENCES_MARGIN_X + ICON_WIDTH + DESCRIPTION_MARGIN)*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, -(boundingRect().height() - PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)), Qt::AlignLeft | Qt::AlignVCenter, tr(EMAIL_TEXT));

        // subrect
        painter->setOpacity(OPACITY_WARNING_BACKGROUND);
        painter->setPen(Qt::NoPen);
        QPainterPath path;
        QRectF rect = boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              WARNING_OFFSET_Y*G_SCALE,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                              -PREFERENCES_MARGIN_Y*G_SCALE);
        path.addRoundedRect(rect, SMALL_ROUNDED_RECT_RADIUS*G_SCALE, SMALL_ROUNDED_RECT_RADIUS*G_SCALE);
        painter->fillPath(path, QBrush(color));
        painter->setPen(Qt::SolidLine);

        // Text inside subrect
        painter->setFont(FontManager::instance().getFont(12,  QFont::Normal));
        QString text;
        if (email_.isEmpty()) {
            painter->setPen(Qt::white);
            painter->setOpacity(OPACITY_SIXTY);
            text = tr(UPGRADE_TEXT);
            painter->drawText(rect.adjusted(DESCRIPTION_MARGIN*G_SCALE, DESCRIPTION_MARGIN*G_SCALE, -DESCRIPTION_MARGIN*G_SCALE, -DESCRIPTION_MARGIN*G_SCALE),
                              Qt::AlignLeft | Qt::TextWordWrap,
                              text);
        } else {
            painter->setPen(QColor(253, 239, 0));
            painter->setOpacity(OPACITY_FULL);
            text = tr(CONFIRM_TEXT);
            painter->drawText(rect.adjusted(DESCRIPTION_MARGIN*G_SCALE, DESCRIPTION_MARGIN*G_SCALE, -DESCRIPTION_MARGIN*G_SCALE - sendButton_->boundingRect().width(), -DESCRIPTION_MARGIN*G_SCALE),
                              Qt::AlignLeft | Qt::TextWordWrap,
                              text);
        }
    } else {
        painter->setPen(Qt::white);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, -(boundingRect().height() - PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)), Qt::AlignLeft | Qt::AlignVCenter, tr(EMAIL_TEXT));

        if (!email_.isEmpty()) {
            painter->setOpacity(OPACITY_SIXTY);
            QFont font = FontManager::instance().getFont(14,  QFont::Normal);
            painter->setFont(font);
            QFontMetrics fmEmail(font);
            QString elidedEmail = fmEmail.elidedText(email_, Qt::TextElideMode::ElideMiddle, boundingRect().width() - (2*PREFERENCES_MARGIN_X + APP_ICON_MARGIN_X)*G_SCALE - textWidth);
            painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, -(boundingRect().height() - PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE)), Qt::AlignRight | Qt::AlignVCenter, elidedEmail);
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
    if (email_.isEmpty()) {
        emptyEmailButton_->setClickable(true);
        if (emailSendState_ != EMAIL_ADDING) {
            emptyEmailButton_->setVisible(true);
        }
        sendButton_->hide();

        QFontMetrics fm(FontManager::instance().getFont(12,  QFont::Normal));
        msgHeight_ = fm.boundingRect(boundingRect().adjusted((PREFERENCES_MARGIN_X + DESCRIPTION_MARGIN)*G_SCALE,
                                                             0,
                                                             -(PREFERENCES_MARGIN_X + DESCRIPTION_MARGIN)*G_SCALE,
                                                             0).toRect(),
                                     Qt::AlignLeft | Qt::TextWordWrap,
                                     tr(UPGRADE_TEXT)).height();

        height_ = (WARNING_OFFSET_Y + 2*DESCRIPTION_MARGIN + PREFERENCES_MARGIN_Y)*G_SCALE + msgHeight_;
        emptyEmailButton_->setPos(boundingRect().width() - emptyEmailButton_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE,
                                  PREFERENCES_MARGIN_Y*G_SCALE);

    } else if (isNeedConfirmEmail_) {
        emptyEmailButton_->setClickable(false);
        emptyEmailButton_->setVisible(false);

        QFontMetrics fm(FontManager::instance().getFont(12,  QFont::Normal));
        msgHeight_ = fm.boundingRect(boundingRect().adjusted((PREFERENCES_MARGIN_X + DESCRIPTION_MARGIN)*G_SCALE,
                                                             0,
                                                             -(PREFERENCES_MARGIN_X + DESCRIPTION_MARGIN)*G_SCALE - sendButton_->boundingRect().width(),
                                                             0).toRect(),
                                     Qt::AlignLeft | Qt::TextWordWrap,
                                     tr(CONFIRM_TEXT)).height();

        height_ = (WARNING_OFFSET_Y + 2*DESCRIPTION_MARGIN + PREFERENCES_MARGIN_Y)*G_SCALE + msgHeight_;
        sendButton_->setPos(boundingRect().width() - sendButton_->boundingRect().width() - (PREFERENCES_MARGIN_X + DESCRIPTION_MARGIN)*G_SCALE,
                            (WARNING_OFFSET_Y + DESCRIPTION_MARGIN)*G_SCALE + (fm.height() - sendButton_->boundingRect().height()) / 2);
        sendButton_->show();
    } else {
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
    if (!isVisible() && (emailSendState_ == EMAIL_SENDING || emailSendState_ == EMAIL_SENT ||  emailSendState_ == EMAIL_FAILED_SEND)) {
        setEmailSendState(EMAIL_NO_SEND);
    }
}

void EmailItem::setEmailSendState(EmailItem::EMAIL_SEND_STATE state)
{
    if (state == EMAIL_NO_SEND) {
        sendButton_->setText(tr("Resend"));
        sendButton_->setClickable(true);
    } else if (state == EMAIL_SENDING) {
        sendButton_->setText(tr("Sending"));
        sendButton_->setClickable(false);
    } else if (state == EMAIL_SENT) {
        sendButton_->setText(tr("Sent!"));
        sendButton_->setClickable(false);
    } else if (state == EMAIL_FAILED_SEND) {
        sendButton_->setText(tr("Failed"));
        sendButton_->setClickable(false);
    }

    emailSendState_ = state;
    updatePositions();
}

void EmailItem::onLanguageChanged()
{
    emptyEmailButton_->setText(tr(ADD_TEXT));
    setEmailSendState(emailSendState_);
    update();
}

void EmailItem::onSpinnerRotationChanged(const QVariant &value)
{
    spinnerRotation_ = value.toInt();
    update();
}

void EmailItem::onSpinnerRotationFinished()
{
    startAnAnimation<int>(spinnerAnimation_, 0, 360, ANIMATION_SPEED_VERY_SLOW);
}

void EmailItem::onEmptyEmailButtonClick()
{
    setEmailSendState(EMAIL_ADDING);
    startAnAnimation<int>(spinnerAnimation_, spinnerRotation_, 360, ANIMATION_SPEED_VERY_SLOW);
    emptyEmailButton_->setVisible(false);
    emit emptyEmailButtonClick();
}

void EmailItem::setWebSessionCompleted()
{
    if (emailSendState_ == EMAIL_ADDING) {
        spinnerAnimation_.stop();
        emptyEmailButton_->setVisible(true);
        setEmailSendState(EMAIL_NO_SEND);
        update();
    }
}

} // namespace PreferencesWindow
