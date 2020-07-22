#ifndef LOCATIONITEM_H
#define LOCATIONITEM_H

#include "iwidgetlocationsinfo.h"

#include <QString>
#include <QPainter>
#include <QElapsedTimer>
#include <QTextLayout>
#include "itemtimems.h"
#include "../Backend/Types/pingtime.h"
#include "../Backend/Types/locationid.h"
#include "citynode.h"

namespace GuiLocations {

class LocationItem
{
public:

    enum CITY_SUBMENU_STATE { EXPANDED, COLLAPSED, EXPANDING, COLLAPSING };

    LocationItem(IWidgetLocationsInfo *widgetLocationsInfo, int id, const QString &countryCode, const QString &name,
                 bool isShowP2P, PingTime timeMs, QVector<CityNode> &cities, bool forceExpand, bool isPremiumOnly);

    ~LocationItem();

    QString getName() const { return name_; }
    int getPingTimeMs() { return timeMs_.getValue().toInt(); }

    // ind -1, no selected
    // ind = 0, location selected
    // ind > 0, city selected
    void setSelected(int ind);
    int getSelected();
    void setSelectedByCity(const LocationID &locationId);

    int countVisibleItems();

    void drawLocationCaption(QPainter *painter, const QRect &rc);
    void drawCities(QPainter *painter, const QRect &rc, QVector<int> &linesY, int &selectedY1, int &selectedY2);

    bool mouseMoveEvent(QPoint &pt);
    void mouseLeaveEvent();
    QPoint getConnectionMeterCenter();
    bool isCursorOverArrow() { return isCursorOverArrow_; }
    bool isCursorOverP2P() { return isCursorOverP2P_; }
    bool isCursorOverConnectionMeter() { return isCursorOverConnectionMeter_; }
    bool isCursorOverFavouriteIcon() { return isCursorOverFavoriteIcon_; }

    // return -1, if cursor no over favorite icon
    // otherwise return city ind, where cursor over favorite icon
    int detectOverFavoriteIconCity();

    CITY_SUBMENU_STATE citySubMenuState() { return citySubMenuState_; }
    void expand();
    void setExpandedWithoutAnimation();
    void collapse();
    void collapseForbidden();

    bool isExpandedOrExpanding();
    bool isForceExpand();

    bool isAnimationFinished();
    int currentAnimationHeight();

    // if ind == 0 return forbidden for location, int >= 1 return forbidden for city
    bool isForbidden(int ind);
    bool isForbidden(const CityNode &cityNode);
    bool isNoConnection(int ind);
    bool isSelectedDisabled();

    int getId() { return id_; }
    QString getCity(int ind);
    int findCityInd(const LocationID &locationId);
    QList<CityNode> cityNodes();
    void changeSpeedConnection(const LocationID &locationId, PingTime timeMs);

    // return true if changed
    bool changeIsFavorite(const LocationID &locationId, bool isFavorite);

    // return current favorite state
    bool toggleFavorite(int cityInd);

    void updateLangauge(QString name);
    void updateScaling();

private:
    const int EXPAND_ANIMATION_DURATION = 200;

    // visible properties
    QString countryCode_;
    QString name_;
    bool isShowP2P_;
    ItemTimeMs timeMs_;
    QVector<CityNode> cityNodes_;
    int id_;
    bool forceExpand_;

    // internal states
    CITY_SUBMENU_STATE citySubMenuState_;
    int selectedInd_;
    bool isCursorOverArrow_;
    bool isCursorOverP2P_;
    bool isPremiumOnly_;
    bool isCursorOverConnectionMeter_;
    bool isCursorOverFavoriteIcon_;
    QElapsedTimer elapsedAnimationTimer_;
    int startAnimationHeight_;
    int endAnimationHeight_;
    int curAnimationDuration_;

    double curWhiteLineValue_;
    double startWhiteLineValue_;
    double endWhiteLineValue_;

    QTextLayout *captionTextLayout_;

    IWidgetLocationsInfo *widgetLocationsInfo_;

    void drawLocationCaption(QPainter *painter, const QRect &rc, bool bSelected);
    void drawCityCaption(QPainter *painter, CityNode &cityNode, const QRect &rc, bool bSelected);
    bool isCursorOverP2P(const QPoint &pt);
    bool isCursorOverConnectionMeter(const QPoint &pt);
    bool isCursorOverFavoriteIcon(const QPoint &pt, const CityNode &cityNode);
    QRect getConnectionMeterIconRect();
    void drawBottomLine(QPainter *painter, int left, int right, int bottom, double whiteValue);
};

} // namespace GuiLocations

#endif // LOCATIONITEM_H
