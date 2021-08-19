#ifndef IWIDGETLOCATIONSINFO_H
#define IWIDGETLOCATIONSINFO_H

#include <QKeyEvent>
#include "types/locationid.h"

class IWidgetLocationsInfo
{
public:
    virtual ~IWidgetLocationsInfo() {}

    virtual void updateScaling() = 0;

    // accenting
    // For consistency sake the terms "accent" and "highlight" should be used for cursor and keypress hovering
    // While "selection" should be used for clicking or key pressing [Enter] on a location
    // IE: "selecting" an item will engage the vpn and an "accented" item will be selected by pressing [enter]
    virtual bool hasAccentItem() = 0;
    virtual LocationID accentedItemLocationId() = 0;
    virtual void accentFirstItem() = 0;
    virtual void setMuteAccentChanges(bool mute) = 0;

    // cursor and viewport
    virtual bool cursorInViewport() = 0;
    virtual void centerCursorOnAccentedItem() = 0;
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

    virtual int gestureScrollingElapsedTime() = 0;

};

#endif // IWIDGETLOCATIONSINFO_H
