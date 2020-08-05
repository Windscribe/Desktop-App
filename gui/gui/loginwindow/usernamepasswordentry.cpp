#include "usernamepasswordentry.h"

#include <QFont>
#include <QPen>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

UsernamePasswordEntry::UsernamePasswordEntry(QString descriptionText, bool password, ScalableGraphicsObject *parent)
    : ClickableGraphicsObject(parent), curDescriptionPosY_(DESCRIPTION_POS_CLICKED),
      descriptionText_(descriptionText), curLinePos_(0),
      curLineActiveColor_(FontManager::instance().getMidnightColor()),
      height_(50), width_(WINDOW_WIDTH), active_(false), curDescriptionOpacity_(OPACITY_HIDDEN),
      curLineEditOpacity_(OPACITY_HIDDEN), curBackgroundLineOpacity_(OPACITY_UNHOVER_DIVIDER),
      curForgroundLineOpacity_(OPACITY_FULL)
{
    connect(&descriptionPosYAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onDescriptionPosYChanged(QVariant)));
    connect(&linePosAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onLinePosChanged(QVariant)));

    userEntryLine_ = new CustomMenuLineEdit();

    QString ss = "background: white; " + FontManager::instance().getMidnightColorSS();
    userEntryLine_->setStyleSheet(ss);
    userEntryLine_->setFrame(false);
    userEntryLine_->setColorScheme(true);

    userEntryProxy_ = new QGraphicsProxyWidget(this);
    userEntryProxy_->setWidget(userEntryLine_);
    userEntryProxy_->setOpacity(curLineEditOpacity_);

    setFocusProxy(userEntryProxy_);

    if (password)
    {
        userEntryLine_->setEchoMode(QLineEdit::Password);
    }

    connect(userEntryLine_, SIGNAL(keyPressed(QKeyEvent*)), this, SIGNAL(keyPressed(QKeyEvent*)));
    setClickable(true);
    clearActiveState();

    updateScaling();
}

QRectF UsernamePasswordEntry::boundingRect() const
{
    return QRectF(0, 0, width_ * G_SCALE, height_ * G_SCALE);
}

void UsernamePasswordEntry::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setPen(FontManager::instance().getMidnightColor());

    painter->save();
    painter->translate(0, DESCRIPTION_TEXT_HEIGHT/2*G_SCALE);
    painter->setFont(*FontManager::instance().getFont(12, true));
    painter->setOpacity(curDescriptionOpacity_ * initOpacity);
    painter->drawText(WINDOW_MARGIN*G_SCALE, curDescriptionPosY_, tr(descriptionText_.toStdString().c_str()));
    painter->restore();

    painter->save();
    drawBottomLine(painter, WINDOW_MARGIN*G_SCALE,  width_* G_SCALE, (height_ - 2)* G_SCALE, curLinePos_ * G_SCALE, curLineActiveColor_); // bring line into drawing region
    painter->restore();

    userEntryProxy_->setOpacity(curLineEditOpacity_ * initOpacity);
}

bool UsernamePasswordEntry::isActive()
{
    return active_;
}

QString UsernamePasswordEntry::getText()
{
    return userEntryLine_->text();
}

void UsernamePasswordEntry::setError(bool error)
{
    QColor color = FontManager::instance().getMidnightColor();
    QString colorSS = FontManager::instance().getMidnightColorSS();

    if (error)
    {
        color = FontManager::instance().getErrorRedColor();
        colorSS = FontManager::instance().getErrorRedColorSS();
    }

    setAcceptHoverEvents(true);

    colorSS += "; background: white";

    userEntryLine_->setStyleSheet(colorSS);
    curLineActiveColor_ = color;

    update();
}

void UsernamePasswordEntry::setOpacityByFactor(double newOpacityFactor)
{
    curDescriptionOpacity_ = newOpacityFactor * OPACITY_UNHOVER_TEXT;
    curLineEditOpacity_ = newOpacityFactor * OPACITY_FULL;

    curBackgroundLineOpacity_ = newOpacityFactor * OPACITY_UNHOVER_DIVIDER;
    curForgroundLineOpacity_ = newOpacityFactor;

    update();
}

void UsernamePasswordEntry::setClickable(bool clickable)
{
    userEntryLine_->setClickable(clickable);

    if (!clickable)
    {
        userEntryLine_->setCursor(Qt::ArrowCursor);
    }
    else
    {
        userEntryLine_->setCursor(Qt::IBeamCursor);
    }
    ClickableGraphicsObject::setClickable(clickable);
}

void UsernamePasswordEntry::setFocus()
{
    userEntryLine_->setFocus();
}

void UsernamePasswordEntry::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    userEntryProxy_->setPos(WINDOW_MARGIN*G_SCALE, 20*G_SCALE);
    userEntryLine_->setFont(*FontManager::instance().getFont(16, false));
    userEntryLine_->setFixedHeight(height_*G_SCALE - userEntryProxy_->pos().y() - 3*G_SCALE); // keep text box within drawing region and above line
    userEntryLine_->setFixedWidth( width_ * G_SCALE - WINDOW_MARGIN*G_SCALE * 2);
    userEntryLine_->updateScaling();
}

void UsernamePasswordEntry::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (clickable_)
    {
        if (pressed_)
        {
            pressed_ = false;
            if (contains(event->pos()))
            {
                activate();
                setFocus();
            }
        }
    }
    else
    {
        event->ignore();
        return;
    }
}

void UsernamePasswordEntry::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    if (!active_ && clickable_) //  prevents pointing cursor on description text once active
    {
        setCursor(Qt::PointingHandCursor);
    }
}

void UsernamePasswordEntry::onDescriptionPosYChanged(const QVariant &value)
{
    curDescriptionPosY_ = value.toInt();

    update();
}

void UsernamePasswordEntry::onLinePosChanged(const QVariant &value)
{
    curLinePos_ = value.toInt();
    update();
}

void UsernamePasswordEntry::clearActiveState()
{
    startAnAnimation<double>(descriptionPosYAnimation_, curDescriptionPosY_, DESCRIPTION_POS_UNCLICKED, ANIMATION_SPEED_FAST);
    startAnAnimation<int>(linePosAnimation_, curLinePos_, 0, ANIMATION_SPEED_FAST);

    setError(false);
    userEntryProxy_->setVisible(false);
    userEntryLine_->setText("");

    active_ = false;
}

void UsernamePasswordEntry::activate()
{
    active_ = true;

    startAnAnimation<double>(descriptionPosYAnimation_, curDescriptionPosY_, DESCRIPTION_POS_CLICKED, ANIMATION_SPEED_FAST);
    startAnAnimation<int>(linePosAnimation_, curLinePos_, width_, ANIMATION_SPEED_FAST);

    setCursor(Qt::ArrowCursor);

    userEntryProxy_->setVisible(true);

    //userEntryLine_->setFocus();

    emit activated();
}

void UsernamePasswordEntry::drawBottomLine(QPainter *painter, int left, int right, int bottom, int whiteLinePos, QColor activeColor)
{
    // backround line
    QPen pen(FontManager::instance().getMidnightColor());
    pen.setWidth((int)(G_SCALE+0.5));
    painter->setPen(pen);
    painter->setOpacity(curBackgroundLineOpacity_);
    painter->drawLine(left, bottom, right, bottom);
    painter->drawLine(left, bottom+1, right, bottom+1); // thicken

    // foreground line -- do not draw if not active
    if (whiteLinePos != 0)
    {
        QPen pen2(activeColor);
        pen2.setWidth((int)(G_SCALE+0.5));
        painter->setPen(pen2);
        painter->setOpacity(curForgroundLineOpacity_);
        painter->drawLine(left, bottom, left + whiteLinePos, bottom);
        painter->drawLine(left, bottom+1, left + whiteLinePos, bottom+1); // thicken
    }
}

}
