#include "logginginwindowitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {


const int LOGIN_BUTTON_WIDTH = 32;
const int LOGIN_BUTTON_HEIGHT = 32;
const int LOGGING_IN_TEXT_OFFSET_Y = 16;
const int LOGIN_SPINNER_OFFSET = 16;

LoggingInWindowItem::LoggingInWindowItem(QGraphicsObject *parent) : ScalableGraphicsObject(parent),
    curSpinnerRotation_(0), animationActive_(false), targetRotation_(360)
{
    connect(&spinnerAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSpinnerRotationChanged(QVariant)));
}

QGraphicsObject *LoggingInWindowItem::getGraphicsObject()
{
    return this;
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

#ifdef Q_OS_WIN
    painter->fillRect(boundingRect(), QBrush(QColor(0, 0, 0)));
#else
    //todo scale
    QColor black = FontManager::instance().getMidnightColor();
    painter->setPen(black);
    painter->setBrush(black);
    painter->drawRoundedRect(boundingRect().adjusted(0,0,0,0), 5*G_SCALE, 5*G_SCALE);
#endif

    // login circle
    QRectF rect = QRectF(0, 0, LOGIN_BUTTON_WIDTH*G_SCALE, LOGIN_BUTTON_HEIGHT*G_SCALE);
    rect.moveTo((LOGIN_WIDTH - LOGIN_BUTTON_WIDTH - WINDOW_MARGIN)*G_SCALE, LOGIN_BUTTON_POS_Y*G_SCALE);

    painter->setPen(QPen(FontManager::instance().getBrightBlueColor()));
    painter->setBrush(QBrush(FontManager::instance().getBrightBlueColor(), Qt::SolidPattern));
    painter->drawEllipse(rect);

    // text
    painter->setFont(*FontManager::instance().getFont(12, false));
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

void LoggingInWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
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

}
