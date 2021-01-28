#ifndef IWIDGETLOCATIONSINFO_H
#define IWIDGETLOCATIONSINFO_H

#include <QKeyEvent>
#include "types/locationid.h"

class IWidgetLocationsInfo
{
public:
    virtual ~IWidgetLocationsInfo() {}

    virtual bool cursorInViewport() = 0;
    virtual bool hasSelection() = 0;
    virtual void centerCursorOnSelectedItem() = 0;

    virtual void centerCursorOnItem(LocationID locId) = 0;
    virtual LocationID selectedItemLocationId() = 0;

    virtual void setFirstSelected() = 0;
    virtual bool isFreeSessionStatus() = 0;
    virtual int getWidth() = 0;
    virtual int getScrollBarWidth() = 0;
    virtual bool isShowLatencyInMs() = 0;

    virtual void handleKeyEvent(QKeyEvent *event) = 0;

    virtual int countVisibleItems() = 0;

};

#endif // IWIDGETLOCATIONSINFO_H
