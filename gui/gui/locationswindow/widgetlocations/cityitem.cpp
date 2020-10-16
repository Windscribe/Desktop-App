#include "cityitem.h"
#include <QPainter>
#include <QStyleOption>
#include <QtMath>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "widgetlocationssizes.h"
#include "dpiscalemanager.h"

namespace GuiLocations {


CityItem::CityItem(IWidgetLocationsInfo *widgetLocationsInfo, const LocationID &locationId,
                   const QString &cityName1ForShow, const QString &cityName2ForShow,
                   const QString &countryCode, PingTime timeMs, bool bShowPremiumStarOnly,
                   bool isShowLatencyMs, const QString &staticIp, const QString &staticIpType,
                   bool isFavorite, bool isDisabled, bool isCustomConfigCorrect,
                   const QString &customConfigType, const QString &customConfigErrorMessage) :
    cityNode_(locationId, cityName1ForShow, cityName2ForShow, countryCode, timeMs,
              bShowPremiumStarOnly, isShowLatencyMs, staticIp, staticIpType, isFavorite,
              isDisabled, isCustomConfigCorrect, customConfigType, customConfigErrorMessage),
    isSelected_(false), isCursorOverConnectionMeter_(false),
    isCursorOverCaption1Text_(false),
    isCursorOverFavouriteIcon_(false),
    widgetLocationsInfo_(widgetLocationsInfo)
{
}

void CityItem::setSelected(bool isSelected)
{
    isSelected_ = isSelected;
}

bool CityItem::isSelected() const
{
    return isSelected_;
}

LocationID CityItem::getLocationId() const
{
    return cityNode_.getLocationId();
}

QString CityItem::getCountryCode() const
{
    return cityNode_.getCountryCode();
}

int CityItem::getPingTimeMs() const
{
    return cityNode_.timeMs().toInt();
}

QString CityItem::getCaption1TruncatedText() const
{
    return cityNode_.getCaption1TextLayout()->text();
}

QString CityItem::getCaption1FullText() const
{
    return cityNode_.caption1FullText();
}

QPoint CityItem::getCaption1TextCenter() const
{
    QRectF rect = cityNode_.getCaption1TextLayout()->boundingRect();
    return QPoint(static_cast<int>(rect.x() + rect.width()/2), static_cast<int>(rect.y() + rect.height()/2));
}

int CityItem::countVisibleItems()
{
    return 1;
}

void CityItem::drawLocationCaption(QPainter *painter, const QRect &rc)
{
    drawCityCaption(painter, cityNode_, rc, isSelected_);
}

// return true if something is changed and need redraw
bool CityItem::mouseMoveEvent(QPoint &pt)
{
    isCursorOverCaption1Text_ = isCursorOverCaption1Text(pt, cityNode_);

    // detect mouse move for favorite icon
    bool bChanged = false;

    QRect rcCity(0, 0, widgetLocationsInfo_->getWidth(), WidgetLocationsSizes::instance().getItemHeight());

    if (rcCity.contains(pt))
    {
        isCursorOverFavouriteIcon_ = isCursorOverFavoriteIcon(pt, cityNode_);
        if (cityNode_.isCursorOverFavoriteIcon() != isCursorOverFavouriteIcon_)
        {
            cityNode_.setCursorOverFavoriteIcon(isCursorOverFavouriteIcon_);
            bChanged = true;
        }

        isCursorOverConnectionMeter_ = isCursorOverConnectionMeter(pt);
        if (cityNode_.isCursorOverConnectionMeter() != isCursorOverConnectionMeter_)
        {
            cityNode_.setCursorOverConnectionMeter(isCursorOverConnectionMeter_);
            bChanged = true;
        }

        bool bOverCaption1 = isCursorOverCaption1Text(QPoint(pt.x(), pt.y()), cityNode_);
        if (cityNode_.isCursorOverCaption1Text() != bOverCaption1)
        {
            bChanged = true;
        }
    }

    return bChanged;
}

void CityItem::mouseLeaveEvent()
{
    cityNode_.setCursorOverFavoriteIcon(false);
}

bool CityItem::detectOverFavoriteIconCity()
{
    return cityNode_.getStaticIp().isEmpty() && cityNode_.isCursorOverFavoriteIcon();
}

bool CityItem::isForbidden() const
{
    return cityNode_.isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus();
}

bool CityItem::isNoConnection() const
{
    /*if (ind == 0)
    {
        return timeMs_.getValue().toConnectionSpeed() == 0;
    }
    else
    {
        return cityNodes_[ind - 1].timeMs().toConnectionSpeed() == 0;
    }*/
    return false;
}

bool CityItem::isDisabled() const
{
    // API returns disabled for all premium locations when free user session
    // Free users should see premium locations as enabled
    return isForbidden() ? false : cityNode_.isDisabled();
}

bool CityItem::isCustomConfigCorrect() const
{
    return cityNode_.isCustomConfigCorrect();
}

QString CityItem::getCustomConfigErrorMessage() const
{
    return cityNode_.getCustomConfigErrorMessage();
}

int CityItem::findCityInd(const LocationID &locationId)
{
    if (locationId.isTopLevelLocation())
    {
        return -1;
    }

    if (cityNode_.getLocationId() == locationId)
    {
        return 0;
    }
    return -1;
}

void CityItem::changeSpeedConnection(const LocationID &locationId, PingTime timeMs)
{
    if (locationId == cityNode_.getLocationId())
    {
        cityNode_.setConnectionSpeed(timeMs);
    }
}

bool CityItem::changeIsFavorite(const LocationID &locationId, bool isFavorite)
{
    if (locationId == cityNode_.getLocationId())
    {
        if (cityNode_.isFavorite() != isFavorite)
        {
            cityNode_.toggleIsFavorite();
            return true;
        }
    }
    return false;
}

bool CityItem::toggleFavorite()
{
    cityNode_.toggleIsFavorite();
    return cityNode_.isFavorite();
}

void CityItem::updateScaling()
{
    cityNode_.updateScaling();
}

void CityItem::drawCityCaption(QPainter *painter, CityNode &cityNode, const QRect &rc, bool bSelected)
{
    painter->save();

    painter->setOpacity(1.0);
    drawBottomLine(painter, rc.left() + 24, rc.right(), rc.bottom(), 0.0);

    if (!cityNode_.getStaticIp().isEmpty())
    {
        IndependentPixmap *datacenterPixmap = NULL;
        if (cityNode.getStaticIpType().compare("dc", Qt::CaseInsensitive) == 0)
        {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/RES_IP_ICON");
        }
        else if (cityNode.getStaticIpType().compare("res", Qt::CaseInsensitive) == 0)
        {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/DATACENTER_ICON");
        }
        else
        {
            Q_ASSERT(false);
        }
        if (datacenterPixmap != NULL)
        {
            int premiumStarPixmapHeight = datacenterPixmap->height();
            datacenterPixmap->draw(rc.left() + 24 * G_SCALE, rc.top() + (rc.height() - premiumStarPixmapHeight) / 2, painter);
        }

        QRectF boundingRectStaticIp = cityNode.getStaticIpLayout()->boundingRect();

        painter->setPen(QColor(0xFF, 0xFF , 0xFF));
        painter->setOpacity(0.5);
        cityNode.getStaticIpLayout()->draw(painter, QPoint(rc.right() - boundingRectStaticIp.width() - 44 * G_SCALE, rc.top() + (rc.height() - boundingRectStaticIp.height()) / 2 + 1*G_SCALE));
    }
    else if (cityNode.isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus())
    {
        IndependentPixmap *premiumStarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_CITY_STAR");
        int premiumStarPixmapHeight = premiumStarPixmap->height();
        premiumStarPixmap->draw(rc.left() + 16 * G_SCALE, rc.top() + (rc.height() - premiumStarPixmapHeight) / 2, painter);
    }
    else
    {
        IndependentPixmap *favoritePixmap;
        if (cityNode.isFavorite())
        {
            favoritePixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/SELECTED_FAV_ICON");
        }
        else
        {
            favoritePixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/DESELECTED_FAV_ICON");
        }

        int favoritePixmapHeight = favoritePixmap->height();
        painter->setOpacity(cityNode.isCursorOverFavoriteIcon() ? 1.0 : 0.5);
        favoritePixmap->draw(rc.left() + 16 * G_SCALE, rc.top() + (rc.height() - favoritePixmapHeight) / 2, painter);
    }

    // caption 1
    painter->setPen(QColor(0xFF, 0xFF , 0xFF));
    painter->setOpacity(bSelected && !cityNode.isDisabled() ? 1.0  : 0.5);
    QRectF boundingRect1 = cityNode.getCaption1TextLayout()->boundingRect();
    cityNode.getCaption1TextLayout()->draw(painter, QPoint(64 * G_SCALE, rc.top() + (rc.height() - boundingRect1.height()) / 2 - 1*G_SCALE));

    // caption 2
    if (cityNode.getStaticIp() == "")
    {
        QRectF boundingRect2 = cityNode.getCaption2TextLayout()->boundingRect();
        cityNode.getCaption2TextLayout()->draw(painter, QPoint(68 * G_SCALE + boundingRect1.width(), rc.top() + (rc.height() - boundingRect2.height()) / 2 - 1*G_SCALE));
    }

    painter->setOpacity(1.0);

    // API returns disabled for all premium locations when viewed as free user
    bool disabled = isForbidden() ? false : cityNode.isDisabled();

    if (disabled)
    {
        painter->setOpacity(.3);
        IndependentPixmap *pingBarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/UNDER_CONSTRUCTION_ICON");

        int pingBarPixmapHeight = pingBarPixmap->height();
        pingBarPixmap->draw(rc.right() - pingBarPixmap->width() - 16 * G_SCALE, rc.top() + (rc.height() - pingBarPixmapHeight) / 2, painter);
    }
    else
    {
        if (widgetLocationsInfo_->isShowLatencyInMs())
        {
            int latencyRectWidth = 33 * G_SCALE;
            int latencyRectHeight = 20 * G_SCALE;
            QRect latencyRect(rc.right() - latencyRectWidth - 8 * G_SCALE, rc.top() + ((rc.height() - latencyRectHeight) / 2 - 1*G_SCALE), latencyRectWidth, latencyRectHeight);

            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(QBrush(QColor(40, 45, 61)));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(latencyRect, 4, 4);

            QRectF boundingRect3 = cityNode.getLatencyMsTextLayout()->boundingRect();
            boundingRect3.moveCenter(latencyRect.center());

            painter->setPen(QColor(255, 255, 255));

            cityNode.getLatencyMsTextLayout()->draw(painter, QPoint(boundingRect3.left() + 2, boundingRect3.top() + 1));

            //cityNode.getLatencyMsTextLayout()->draw(painter, QPoint(rc.right() - boundingRect3.width() - 16, rc.top() + (rc.height() - boundingRect3.height()) / 2 - 1));
        }
        else
        {
            IndependentPixmap *pingBarPixmap = nullptr;
            if (cityNode.timeMs().toConnectionSpeed() == 0)
            {
                pingBarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/LOCATION_PING_BARS0");
                painter->setOpacity(0.5);
            }
            else if (cityNode.timeMs().toConnectionSpeed() == 1)
            {
                pingBarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/LOCATION_PING_BARS1");
            }
            else if (cityNode.timeMs().toConnectionSpeed() == 2)
            {
                pingBarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/LOCATION_PING_BARS2");
            }
            else if (cityNode.timeMs().toConnectionSpeed() == 3)
            {
                pingBarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/LOCATION_PING_BARS3");
            }
            else
            {
                Q_ASSERT(false);
            }
            int pingBarPixmapHeight = pingBarPixmap->height();
            pingBarPixmap->draw(rc.right() - pingBarPixmap->width() - 16 * G_SCALE, rc.top() + (rc.height() - pingBarPixmapHeight) / 2, painter);
        }
    }

    painter->restore();
}

bool CityItem::isCursorOverArrow(const QPoint &pt)
{
    Q_UNUSED(pt);
    /*if (cityNodes_.count() > 0 )
    {
        int rightOffs = SPACE_FROM_RIGHT;
        QPixmap pixmapArrow;
        pixmapArrow = ImageResources::instance().getLocationArrowDownLight();
        int pixmapArrowWidth = pixmapArrow.width() / pixmapArrow.devicePixelRatio();
        int pixmapArrowHeight = pixmapArrow.height() / pixmapArrow.devicePixelRatio();
        rightOffs += pixmapArrow.width() / pixmapArrow.devicePixelRatio();

        int width = 338;
        int height = WidgetLocationsSizes::instance().getItemHeight();
        QRect rc(width - rightOffs, (height - pixmapArrowHeight) / 2, pixmapArrowWidth, pixmapArrowHeight);
        rc = rc.marginsAdded(QMargins(3, 3, 3, 3));
        return rc.contains(pt);
    }
    else*/
    {
        return false;
    }
}

bool CityItem::isCursorOverP2P(const QPoint &pt)
{
    Q_UNUSED(pt);

    return false;
}

bool CityItem::isCursorOverConnectionMeter(const QPoint &pt)
{
    IndependentPixmap *pingMeterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/LOCATION_PING_BARS0");

    QRect rc(333*G_SCALE - pingMeterPixmap->width() - 16 * G_SCALE,
             17*G_SCALE,
             pingMeterPixmap->width(),
             pingMeterPixmap->height());

    return rc.contains(pt);
}

bool CityItem::isCursorOverFavoriteIcon(const QPoint &pt, const CityNode &cityNode)
{
    if (cityNode.isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus())
    {
        return false;
    }

    IndependentPixmap *favoritePixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/DESELECTED_FAV_ICON");
    int favoritePixmapWidth =  static_cast<int>(favoritePixmap->width() );
    int favoritePixmapHeight = static_cast<int>(favoritePixmap->height());

    QRect rc(16 * G_SCALE, (48 * G_SCALE - favoritePixmapHeight) / 2, favoritePixmapWidth, favoritePixmapHeight);
    return rc.contains(pt);
}

bool CityItem::isCursorOverCaption1Text(const QPoint &pt, const CityNode &cityNode)
{
    QRectF boundingRect1 = cityNode.getCaption1TextLayout()->boundingRect();

    QRect rc(64 * G_SCALE, 15 * G_SCALE, boundingRect1.width(), boundingRect1.height());
    return rc.contains(pt);
}

void CityItem::drawBottomLine(QPainter *painter, int left, int right, int bottom, double whiteValue)
{
    // draw white line with whiteValue from 0.0 to 1.0
    if( qFabs(1.0 - whiteValue) < 0.000001 )
    {
        QPen pen(Qt::white);
        pen.setWidth(1);
        painter->setPen(pen);
        painter->drawLine(left, bottom, right, bottom);
        painter->drawLine(left, bottom - 1, right, bottom - 1);
    }
    else
    {
        QPen pen(QColor(0x29, 0x2E, 0x3E));
        pen.setWidth(1);
        painter->setPen(pen);
        painter->drawLine(left, bottom, right, bottom);
        painter->drawLine(left, bottom - 1, right, bottom - 1);

        if (whiteValue > 0.000001 )
        {
            int w = static_cast<int>((right - left) * whiteValue);
            QPen white_pen(Qt::white);
            white_pen.setWidth(1);
            painter->setPen(white_pen);
            painter->drawLine(left, bottom, left + w, bottom);
            painter->drawLine(left, bottom - 1, left + w, bottom - 1);
        }
    }
}

} // namespace GuiLocations
