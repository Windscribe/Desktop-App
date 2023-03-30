#include "clickablegraphicsobject.h"

#include <QCursor>
#include <QTimer>
#include <QGraphicsSceneMouseEvent>
#include "languagecontroller.h"


ClickableGraphicsObject::ClickableGraphicsObject(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
    selected_(false), stickySelection_(false), clickable_(false), hoverable_(false), resetHoverOnClick_(true), pressed_(false), hovered_(false)
{
    // QtBug: default clickable_(false) because a call to setClickable (setCursor) in the constructor will cause SIGABORT
    setAcceptHoverEvents(false);
    setZValue(0);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ClickableGraphicsObject::onLanguageChanged);
}

bool ClickableGraphicsObject::isClickable()
{
    return clickable_;
}

void ClickableGraphicsObject::setClickable(bool clickable)
{
    clickable_ = clickable;
    hoverable_ = clickable;

    setAcceptHoverEvents(clickable);

    QCursor cursor = Qt::ArrowCursor;
    if (clickable) cursor = Qt::PointingHandCursor;
    setCursor(cursor);
}

void ClickableGraphicsObject::setClickableHoverable(bool clickable, bool hoverable)
{
    clickable_ = clickable;
    hoverable_ = hoverable;

    setAcceptHoverEvents(hoverable);

    QCursor cursor = Qt::ArrowCursor;
    if (clickable) cursor = Qt::PointingHandCursor;
    setCursor(cursor);
}

void ClickableGraphicsObject::setResetHoverOnClick(bool do_reset)
{
    resetHoverOnClick_ = do_reset;
}

bool ClickableGraphicsObject::isSelected()
{
    return selected_;
}

void ClickableGraphicsObject::setSelected(bool selected)
{
    selected_ = selected;
}

void ClickableGraphicsObject::setStickySelection(bool stickySelection)
{
    stickySelection_ = stickySelection;
}

void ClickableGraphicsObject::onLanguageChanged()
{
    // overwrite

}

void ClickableGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (clickable_)
    {
        if (event->button() == Qt::LeftButton)
        {
            pressed_ = true;

            //emit clicked();
            //event->ignore();
        }
    }
    else
    {
        event->ignore();
    }
}

void ClickableGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (pressed_)
        {
            pressed_ = false;

            if (contains(event->pos()))
            {
                emit clicked();
                if (resetHoverOnClick_)
                    hoverLeaveEvent(nullptr);
            }
        }
    }
}

void ClickableGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    if (hoverable_)
    {
        selected_ = true;
        hovered_ = true;
        emit hoverEnter();
    }
}

void ClickableGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    if (hovered_)
    {
        if (!stickySelection_)
        {
            selected_ = false;
        }
        hovered_ = false;
        emit hoverLeave();
    }
}
