#ifndef CONFIGFOOTERITEM_H
#define CONFIGFOOTERITEM_H

#include "icityitem.h"

namespace GuiLocations {

class ConfigFooterItem : public ICityItem
{
    Q_INTERFACES(ICityItem)
public:
    ConfigFooterItem(IWidgetLocationsInfo *widgetLocationsInfo);
    void setSelected(bool isSelected)                            override;
    bool isSelected()                                            override;
    LocationID getLocationId()                                   override;
    QString getCountryCode() const                               override;
    int getPingTimeMs()                                          override;
    QString getCaption1TruncatedText()                           override;
    QString getCaption1FullText()                                override;
    QPoint getCaption1TextCenter()                               override;
    int countVisibleItems()                                      override;
    void drawLocationCaption(QPainter *painter, const QRect &rc) override;

    bool mouseMoveEvent(QPoint &pt)    override;
    void mouseLeaveEvent()             override;
    bool isCursorOverConnectionMeter() override;
    bool isCursorOverCaption1Text()    override;
    bool isCursorOverFavouriteIcon()   override;
    bool detectOverFavoriteIconCity()  override;

    // if ind == 0 return forbidden for location, int >= 1 return forbidden for city
    bool isForbidden()    override;
    bool isNoConnection() override;
    bool isDisabled()     override;

    int findCityInd(const LocationID &locationId) override;
    void changeSpeedConnection(const LocationID &locationId, PingTime timeMs) override;

    // return true if changed
    bool changeIsFavorite(const LocationID &locationId, bool isFavorite) override;
    bool toggleFavorite() override;     // return current favorite state
    void updateScaling() override;

private:
    bool isSelected_;
    bool isCursorOverCaption1Text_;
    IWidgetLocationsInfo *widgetLocationsInfo_;
};

}

#endif // CONFIGFOOTERITEM_H
