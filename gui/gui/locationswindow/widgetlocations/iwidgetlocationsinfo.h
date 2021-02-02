#ifndef IWIDGETLOCATIONSINFO_H
#define IWIDGETLOCATIONSINFO_H

#include <QKeyEvent>
#include "types/locationid.h"

// TODO: fix touch screen gestures
// TODO: but when scrolling to end of list will mess up the notching

class IWidgetLocationsInfo
{
public:
    virtual ~IWidgetLocationsInfo() {}

    // TODO: bug when changing scales when displaying lower in the list
    virtual void updateScaling() = 0;

    // selection
    virtual bool hasSelection() = 0;
    virtual LocationID selectedItemLocationId() = 0;
    virtual void setFirstSelected() = 0;

    // cursor and viewport
    virtual bool cursorInViewport() = 0;
    virtual void centerCursorOnSelectedItem() = 0;
    virtual void centerCursorOnItem(LocationID locId) = 0;
    virtual int countViewportItems() = 0;
    virtual void setCountViewportItems(int count) = 0;

    // state
    virtual bool isShowLatencyInMs() = 0;
    virtual void setShowLatencyInMs(bool showLatencyInMs) = 0;
    virtual bool isFreeSessionStatus() = 0;

    // other
    virtual void startAnimationWithPixmap(const QPixmap &pixmap) = 0;
    virtual void handleKeyEvent(QKeyEvent *event) = 0;


};

#endif // IWIDGETLOCATIONSINFO_H
