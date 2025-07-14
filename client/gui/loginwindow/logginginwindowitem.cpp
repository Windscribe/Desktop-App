#include "logginginwindowitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"

namespace LoginWindow {


const int LOGIN_BUTTON_WIDTH = 32;
const int LOGIN_BUTTON_HEIGHT = 32;
const int LOGGING_IN_TEXT_OFFSET_Y = 16;
const int LOGIN_SPINNER_OFFSET = 16;


LoggingInWindowItem::LoggingInWindowItem(QGraphicsObject *parent) : ScalableGraphicsObject(parent),
    curSpinnerRotation_(0), animationActive_(false), targetRotation_(360)
{
    backButton_ = new IconButton(32, 32, "BACK_ARROW", "", this);
    connect(backButton_, &IconButton::clicked, this, &LoggingInWindowItem::onBackClicked);
    backButton_->setVisible(false);

    connect(&spinnerAnimation_, &QVariantAnimation::valueChanged, this, &LoggingInWindowItem::onSpinnerRotationChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &LoggingInWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QRectF LoggingInWindowItem::boundingRect() const
{
    return QRectF(0, 0, LOGIN_WIDTH*G_SCALE, LOGIN_HEIGHT*G_SCALE);
}

void LoggingInWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    QColor color = FontManager::instance().getMidnightColor();
    painter->setPen(color);
    painter->setBrush(color);
    painter->drawRoundedRect(boundingRect(), 9*G_SCALE, 9*G_SCALE);

    // draw captcha screen
    if (captchaItem_) {
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(9, 15, 25));
        painter->setPen(Qt::SolidLine);

        QRectF rcBottomRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE, (LOGIN_HEIGHT - HEADER_HEIGHT)*G_SCALE);

        QColor darkblue = FontManager::instance().getDarkBlueColor();
        painter->setBrush(darkblue);
        painter->setPen(darkblue);
        painter->drawRoundedRect(rcBottomRect, 9*G_SCALE, 9*G_SCALE);

        // We don't actually want the upper corners of the bottom rect to be rounded.  Fill them in
        painter->fillRect(QRect(0, HEADER_HEIGHT*G_SCALE, WINDOW_WIDTH*G_SCALE, 9*G_SCALE), darkblue);

        // Login Text
        painter->setFont(FontManager::instance().getFont(24,  QFont::Normal));
        painter->setPen(QColor(255,255,255)); //  white

        QString loginText = tr("Login");
        QFontMetrics fm = painter->fontMetrics();
        const int loginTextWidth = fm.horizontalAdvance(loginText);
        painter->drawText(centeredOffset(WINDOW_WIDTH*G_SCALE, loginTextWidth),
                          (HEADER_HEIGHT/2 + 16)*G_SCALE,
                          loginText);

        // Complete Puzzle text
        QFont font = FontManager::instance().getFont(16,  QFont::Bold);
        painter->setFont(font);
        painter->setPen(QColor(255,255,255)); //  white
        painter->drawText(QRectF(20 * G_SCALE, HEADER_HEIGHT * G_SCALE, (WINDOW_WIDTH-40)*G_SCALE, captchaItem_->pos().y() - HEADER_HEIGHT * G_SCALE),
                          Qt::AlignCenter | Qt::TextWordWrap, puzzleText_);

    } else {    // draw logging in screen
        // login circle
        QRectF rect = QRectF(0, 0, LOGIN_BUTTON_WIDTH*G_SCALE, LOGIN_BUTTON_HEIGHT*G_SCALE);
        rect.moveTo((LOGIN_WIDTH - LOGIN_BUTTON_WIDTH - WINDOW_MARGIN)*G_SCALE, LOGIN_BUTTON_POS_Y*G_SCALE);

        painter->setPen(QPen(FontManager::instance().getBrightBlueColor()));
        painter->setBrush(QBrush(FontManager::instance().getBrightBlueColor(), Qt::SolidPattern));
        painter->drawEllipse(rect);

        // text
        painter->setFont(FontManager::instance().getFont(12,  QFont::Normal));
        painter->setPen(QColor(255,255,255));
        painter->drawText(WINDOW_MARGIN*G_SCALE, (LOGIN_BUTTON_POS_Y+LOGGING_IN_TEXT_OFFSET_Y)*G_SCALE, message_);

        if (additionalMessage_ != "")
        {
            const int LOGGING_IN_ADDITIONAL_TEXT_POS_Y = 292;
            painter->drawText(WINDOW_MARGIN*G_SCALE, LOGGING_IN_ADDITIONAL_TEXT_POS_Y*G_SCALE, tr(additionalMessage_.toStdString().c_str()));
        }

        // spinner
        painter->save();
        painter->translate((LOGIN_WIDTH - LOGIN_BUTTON_WIDTH - WINDOW_MARGIN +LOGIN_SPINNER_OFFSET)*G_SCALE, (LOGIN_BUTTON_POS_Y+LOGIN_SPINNER_OFFSET)*G_SCALE);
        painter->rotate(curSpinnerRotation_);
        QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("SPINNER");
        pixmap->draw(-LOGIN_SPINNER_OFFSET/2*G_SCALE, -LOGIN_SPINNER_OFFSET/2*G_SCALE, painter);
        painter->restore();
    }
}

void LoggingInWindowItem::setClickable(bool enabled)
{
    if (captchaItem_) {
        backButton_->setClickable(enabled);
        captchaItem_->setClickable(enabled);
    }
}

void LoggingInWindowItem::startAnimation()
{
    animationActive_ = true;

    curSpinnerRotation_ = 0;
    targetRotation_ = 360;

    startAnAnimation<int>(spinnerAnimation_, curSpinnerRotation_, targetRotation_, ANIMATION_SPEED_VERY_SLOW);
}

void LoggingInWindowItem::stopAnimation()
{
    animationActive_ = false;

    spinnerAnimation_.stop();
}

void LoggingInWindowItem::setMessage(const QString &msg)
{
    message_ = msg;
    update();
}

void LoggingInWindowItem::setAdditionalMessage(const QString &msg)
{
    additionalMessage_ = msg;
    update();
}

void LoggingInWindowItem::showCaptcha(const QString &background, const QString &slider, int top)
{
    SAFE_DELETE(captchaItem_);
    captchaItem_ = new CaptchaItem(this, background, slider, top);
    captchaItem_->setClickable(true);
    connect(captchaItem_, &CaptchaItem::captchaResolved, this, &LoggingInWindowItem::onCaptchaResolved);
    backButton_->setVisible(true);
    updatePositions();
    update();
}

void LoggingInWindowItem::hideCaptcha()
{
    SAFE_DELETE_LATER(captchaItem_);
    backButton_->setVisible(false);
}

void LoggingInWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void LoggingInWindowItem::onSpinnerRotationChanged(const QVariant &value)
{
    curSpinnerRotation_ = value.toInt();

    // restart until stopped
    if (animationActive_ && (abs(curSpinnerRotation_ - targetRotation_)) < 5) //  close enough
    {
        startAnimation();
    }

    update();
}

void LoggingInWindowItem::onCaptchaResolved(const QString &captchaSolution, const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY)
{
    hideCaptcha();
    emit captchaResolved(captchaSolution, captchaTrailX, captchaTrailY);
}

void LoggingInWindowItem::onBackClicked()
{
    emit backClicked();
}

void LoggingInWindowItem::onLanguageChanged()
{
    puzzleText_ = tr("Complete Puzzle to continue");
    updatePositions();
}

void LoggingInWindowItem::updatePositions()
{
    if (captchaItem_) {
        backButton_->setPos(16*G_SCALE, 28*G_SCALE);

        QFont font = FontManager::instance().getFont(16,  QFont::Bold);
        auto puzzleTextRect = CommonGraphics::textBoundingRect(font, 0, 0, (WINDOW_WIDTH - 40)*G_SCALE,  puzzleText_, Qt::TextWordWrap);

        auto rc = captchaItem_->boundingRect();
        int y_offs = ((LOGIN_HEIGHT - HEADER_HEIGHT)*G_SCALE - puzzleTextRect.height() - rc.height()) / 3;
        captchaItem_->setPos((LOGIN_WIDTH*G_SCALE - rc.width()) / 2, LOGIN_HEIGHT*G_SCALE - rc.height() - y_offs);
    }
}

int LoggingInWindowItem::centeredOffset(int background_length, int graphic_length)
{
    return background_length/2 - graphic_length/2;
}

}
