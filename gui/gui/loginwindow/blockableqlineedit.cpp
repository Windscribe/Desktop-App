#include "blockableqlineedit.h"

// #include <QDebug>

BlockableQLineEdit::BlockableQLineEdit(QWidget *parent) : QLineEdit (parent)
{
    clickable_ = true;
}

void BlockableQLineEdit::setClickable(bool clickable)
{
    clickable_ = clickable;
}

void BlockableQLineEdit::mouseMoveEvent(QMouseEvent *event)
{
    if (!clickable_)
    {
        event->ignore();
        return;
    }

    QLineEdit::mouseMoveEvent(event);
}

void BlockableQLineEdit::mousePressEvent(QMouseEvent *event)
{
    if (!clickable_)
    {
        event->ignore();
        return;
    }

    QLineEdit::mousePressEvent(event);
}

void BlockableQLineEdit::keyPressEvent(QKeyEvent *event)
{
    //qDebug() << "BlockableQLineEdit::keyPressEvent";

    if (!clickable_)
    {
        event->ignore();
        return;
    }

    QLineEdit::keyPressEvent(event);
}

void BlockableQLineEdit::keyReleaseEvent(QKeyEvent *event)
{
    //qDebug() << "BlockableQLineEdit::keyReleaseEvent";
    if (!clickable_)
    {
        event->ignore();
        return;
    }

    QLineEdit::keyReleaseEvent(event);
}

void BlockableQLineEdit::focusInEvent(QFocusEvent *event )
{
    if (!clickable_)
    {
        event->ignore();
        return;
    }

    QLineEdit::focusInEvent(event);
    emit focusIn();
}

void BlockableQLineEdit::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);
    emit focusOut();
}

void BlockableQLineEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!clickable_)
    {
        event->ignore();
        return;
    }

    QLineEdit::mouseDoubleClickEvent(event);
}
