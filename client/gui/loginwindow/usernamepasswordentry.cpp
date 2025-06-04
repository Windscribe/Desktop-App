#include "usernamepasswordentry.h"

#include <QFont>
#include <QPen>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

UsernamePasswordEntry::UsernamePasswordEntry(const QString &text, bool password, ScalableGraphicsObject *parent)
    : ClickableGraphicsObject(parent),
      userEntryLineAddSS_("padding-left: 8px; background: #0cffffff; border-radius: 3px;"),
      descriptionText_(text), height_(50),
      width_(WINDOW_WIDTH), curDescriptionOpacity_(OPACITY_HIDDEN),
      curLineEditOpacity_(OPACITY_HIDDEN)
{
    userEntryLine_ = new CommonWidgets::CustomMenuLineEdit();

    QString ss = userEntryLineAddSS_ + " color: white";
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
        userEntryLine_->setShowRevealToggle(true);
    }

    connect(userEntryLine_, &CommonWidgets::CustomMenuLineEdit::textChanged, this, &UsernamePasswordEntry::textChanged);
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

    painter->setPen(Qt::white);

    if (!descriptionText_.isEmpty()) {
        painter->save();
        painter->translate(0, DESCRIPTION_TEXT_HEIGHT / 2 * G_SCALE);
        painter->setFont(FontManager::instance().getFont(12, QFont::Bold));
        painter->setOpacity(curDescriptionOpacity_ * initOpacity);
        painter->drawText(WINDOW_MARGIN*G_SCALE, 0, tr(descriptionText_.toStdString().c_str()));
        painter->restore();
    }

    userEntryProxy_->setOpacity(curLineEditOpacity_ * initOpacity);
}

QString UsernamePasswordEntry::getText() const
{
    return userEntryLine_->text();
}

void UsernamePasswordEntry::setError(bool error)
{
    QColor color = Qt::white;
    QString colorSS = "color: white";

    if (error)
    {
        color = FontManager::instance().getErrorRedColor();
        colorSS = FontManager::instance().getErrorRedColorSS();
    }

    setAcceptHoverEvents(true);

    colorSS += ";" + userEntryLineAddSS_;

    userEntryLine_->setStyleSheet(colorSS);

    update();
}

void UsernamePasswordEntry::setOpacityByFactor(double newOpacityFactor)
{
    curDescriptionOpacity_ = newOpacityFactor * OPACITY_FULL;
    curLineEditOpacity_ = newOpacityFactor * OPACITY_FULL;
    userEntryProxy_->setOpacity(curLineEditOpacity_);

    update();
}

void UsernamePasswordEntry::setClickable(bool clickable)
{
    userEntryLine_->setClickable(clickable);
    ClickableGraphicsObject::setClickable(clickable);
    setCursor(Qt::ArrowCursor);
}

void UsernamePasswordEntry::setFocus()
{
    userEntryLine_->setFocus();
}

void UsernamePasswordEntry::setWidth(int width)
{
    width_ = width;
    userEntryLine_->setFixedWidth(width_ * G_SCALE - WINDOW_MARGIN * G_SCALE * 2);
}

void UsernamePasswordEntry::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    userEntryProxy_->setPos(WINDOW_MARGIN*G_SCALE, 20*G_SCALE);
    userEntryLine_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    userEntryLine_->setFixedHeight(height_*G_SCALE - userEntryProxy_->pos().y()); // keep text box within drawing region and above line
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

void UsernamePasswordEntry::clearActiveState()
{
    setError(false);
    userEntryLine_->setText("");
}

void UsernamePasswordEntry::setText(const QString &text)
{
    descriptionText_ = text;
    update();
}

}
