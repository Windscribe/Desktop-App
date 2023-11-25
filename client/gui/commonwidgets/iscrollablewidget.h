#pragma once

#include <QWidget>

class IScrollableWidget
{
public:
    virtual ~IScrollableWidget() {}
    virtual QWidget *getWidget() = 0;

    virtual QSize sizeHint() const = 0;
    virtual void setViewportHeight(int height) = 0;

    virtual int stepSize() const = 0;
    virtual void setStepSize(int stepSize) = 0;

signals:
    virtual void heightChanged(int newHeight) = 0;
    virtual void shiftPos(int delta) = 0;

    virtual void recenterWidget() = 0;

};

Q_DECLARE_INTERFACE(IScrollableWidget, "IScrollableWidget")
