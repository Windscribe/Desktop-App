#ifndef CITYITEMABSTRACT_H
#define CITYITEMABSTRACT_H

#include "iwidgetlocationsinfo.h"

#include <QString>
#include <QPainter>
#include <QElapsedTimer>
#include <QTextLayout>
#include "itemtimems.h"
#include "../backend/types/pingtime.h"
#include "types/locationid.h"
#include "citynode.h"

namespace GuiLocations {

class ICityItem
{
public:
    virtual ~ICityItem() {}

    virtual void setSelected(bool isSelected)                            = 0;
    virtual bool isSelected()                                            = 0;
    virtual LocationID getLocationId()                                   = 0;
    virtual QString getCountryCode() const                               = 0;
    virtual int getPingTimeMs()                                          = 0;
    virtual QString getCaption1TruncatedText()                           = 0;
    virtual QString getCaption1FullText()                                = 0;
    virtual QPoint getCaption1TextCenter()                               = 0;
    virtual int countVisibleItems()                                      = 0;
    virtual void drawLocationCaption(QPainter *painter, const QRect &rc) = 0;

    virtual bool mouseMoveEvent(QPoint &pt) = 0;
    virtual void mouseLeaveEvent() = 0;
    virtual bool isCursorOverConnectionMeter() = 0;
    virtual bool isCursorOverCaption1Text() = 0;
    virtual bool isCursorOverFavouriteIcon() = 0;

    virtual bool detectOverFavoriteIconCity() = 0;

    // if ind == 0 return forbidden for location, int >= 1 return forbidden for city
    virtual bool isForbidden()    = 0;
    virtual bool isNoConnection() = 0;
    virtual bool isDisabled()     = 0;

    virtual int findCityInd(const LocationID &locationId) = 0;
    virtual void changeSpeedConnection(const LocationID &locationId, PingTime timeMs) = 0;

    // return true if changed
    virtual bool changeIsFavorite(const LocationID &locationId, bool isFavorite) = 0;

    // return current favorite state
    virtual bool toggleFavorite() = 0;

    virtual void updateScaling() = 0;

};

}

Q_DECLARE_INTERFACE(GuiLocations::ICityItem, "ICityItem")

#endif // CITYITEMABSTRACT_H
