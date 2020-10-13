#ifndef CITYNODE_H
#define CITYNODE_H

#include <QTextLayout>
#include "../backend/types/locationid.h"
#include "itemtimems.h"

namespace GuiLocations {

struct CityNode
{
    CityNode();
    CityNode(const LocationID &locationId, const QString &cityName1ForShow, const QString &cityName2ForShow, const QString &countryCode, PingTime timeMs, bool bShowPremiumStarOnly,
             bool isShowLatencyMs, const QString &staticIp, const QString &staticIpType, bool isFavorite, bool isDisabled);

    void setConnectionSpeed(PingTime timeMs);
    bool isShowPremiumStar() const;
    PingTime timeMs() const;
    QTextLayout *getCaption1TextLayout() const;
    QTextLayout *getCaption2TextLayout() const;
    QTextLayout *getStaticIpLayout() const;
    QTextLayout *getLatencyMsTextLayout();

    QString getStaticIp() const { return staticIp_; }
    QString getStaticIpType() const { return staticIpType_; }

    LocationID getLocationId() const;
    QString getCountryCode() const;

    QString caption1FullText();

    bool isCursorOverFavoriteIcon() const;
    void setCursorOverFavoriteIcon(bool b);

    bool isCursorOverCaption1Text() const;
    void setCursorOverCaption1Text(bool b);

    bool isCursorOverConnectionMeter() const;
    void setCursorOverConnectionMeter(bool b);

    bool isFavorite() const;
    void toggleIsFavorite();

    bool isDisabled() const;
    void updateScaling();

private:
    QString getShortenedCaptionText(QString original, QFont font) const;
    static constexpr int MAXIMUM_CAPTION_WIDTH = 175;

    LocationID locationId_;
    QString countryCode_;
    ItemTimeMs timeMs_;
    bool bShowPremiumStarOnly_;
    QSharedPointer<QTextLayout> caption1TextLayout_;
    QSharedPointer<QTextLayout> caption2TextLayout_;
    QSharedPointer<QTextLayout> captionStaticIpLayout_;
    QString staticIp_;
    QString staticIpType_;
    bool isCursorOverFavoriteIcon_;
    bool isFavorite_;
    bool isCursorOverCaption1Text_;
    bool isDisabled_;
    bool isCursorOverConnectionMeter_;

    QString caption1FullText_;
};

} // namespace GuiLocations

#endif // CITYNODE_H
