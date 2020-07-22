#ifndef CITYITEM_H
#define CITYITEM_H

#include "iwidgetlocationsinfo.h"
#include "icityitem.h"
#include <QString>
#include <QPainter>
#include <QElapsedTimer>
#include <QTextLayout>
#include "itemtimems.h"
#include "../Backend/Types/pingtime.h"
#include "../Backend/Types/locationid.h"
#include "citynode.h"

namespace GuiLocations {

class CityItem : public ICityItem
{
    Q_INTERFACES(ICityItem)

public:
    CityItem(IWidgetLocationsInfo *widgetLocationsInfo, const LocationID &locationId, const QString &cityName1ForShow, const QString &cityName2ForShow, const QString &countryCode,
             PingTime timeMs, bool bShowPremiumStarOnly, bool isShowLatencyMs, const QString &staticIp, const QString &staticIpType, bool isFavorite, bool isDisabled);

    // ind -1, no selected
    // ind = 0, location selected
    // ind > 0, city selected
    void setSelected(bool isSelected) override;
    bool isSelected() override;

    LocationID getLocationId() override;
    QString getCountryCode() const override;
    int getPingTimeMs() override;

    QString getCaption1TruncatedText() override;

    QString getCaption1FullText() override;
    QPoint getCaption1TextCenter() override;

    int countVisibleItems() override;

    void drawLocationCaption(QPainter *painter, const QRect &rc) override;

    bool mouseMoveEvent(QPoint &pt) override;
    void mouseLeaveEvent() override;
    bool isCursorOverConnectionMeter()  override { return isCursorOverConnectionMeter_; }
    bool isCursorOverCaption1Text() override { return isCursorOverCaption1Text_; }
    bool isCursorOverFavouriteIcon() override { return isCursorOverFavouriteIcon_; }
    bool detectOverFavoriteIconCity() override;

    // if ind == 0 return forbidden for location, int >= 1 return forbidden for city
    bool isForbidden() override;
    bool isNoConnection() override;
    bool isDisabled() override;

    int findCityInd(const LocationID &locationId) override;
    void changeSpeedConnection(const LocationID &locationId, PingTime timeMs) override;

    // return true if changed
    bool changeIsFavorite(const LocationID &locationId, bool isFavorite) override;

    // return current favorite state
    bool toggleFavorite() override;
    void updateScaling() override;

private:
    CityNode cityNode_;

    // internal states
    bool isSelected_;
    bool isCursorOverConnectionMeter_;
    bool isCursorOverCaption1Text_;
    bool isCursorOverFavouriteIcon_;

    IWidgetLocationsInfo *widgetLocationsInfo_;

    void drawCityCaption(QPainter *painter, CityNode &cityNode, const QRect &rc, bool bSelected);
    bool isCursorOverArrow(const QPoint &pt);
    bool isCursorOverP2P(const QPoint &pt);
    bool isCursorOverConnectionMeter(const QPoint &pt);
    bool isCursorOverFavoriteIcon(const QPoint &pt, const CityNode &cityNode);
    bool isCursorOverCaption1Text(const QPoint &pt, const CityNode &cityNode);
    void drawBottomLine(QPainter *painter, int left, int right, int bottom, double whiteValue);
};

} // namespace GuiLocations

#endif // CITYITEM_H
