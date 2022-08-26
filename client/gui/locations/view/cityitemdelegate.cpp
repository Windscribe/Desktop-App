#include "cityitemdelegate.h"

#include <QPainter>
#include <QtMath>

#include "../locationsmodel_roles.h"
#include "clickableandtooltiprects.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "types/locationid.h"
#include "types/pingtime.h"
#include "textpixmap.h"

namespace  {

class CityItemDelegateCache : public gui_locations::IItemCacheData, public gui_locations::TextPixmaps
{
public:
    enum {
          kCityId = 0,
          kNickId,
          kStaticIpId
    };
};
}

namespace gui_locations {

void CityItemDelegate::paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const
{
    painter->save();

    double initOpacity = painter->opacity();
    int left_offs = option.rect.left();
    int top_offs = option.rect.top();
    double textOpacity = OPACITY_HALF + (OPACITY_FULL - OPACITY_HALF) * option.selectedOpacity();

    const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);

    // background
    painter->fillRect(option.rect, FontManager::instance().getMidnightColor());

    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (lid.isStaticIpsLocation())
    {
        QString staticIpType = index.data(kStaticIpType).toString();
        // static ip icon
        QSharedPointer<IndependentPixmap> datacenterPixmap;
        if (staticIpType.compare("dc", Qt::CaseInsensitive) == 0)
        {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/DATACENTER_ICON");
        }
        else if (staticIpType.compare("res", Qt::CaseInsensitive) == 0)
        {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/RES_IP_ICON");
        }
        else
        {
            Q_ASSERT(false);
        }

        if (datacenterPixmap)
        {
            datacenterPixmap->draw(left_offs + LOCATION_ITEM_MARGIN_TO_LINE * G_SCALE,
                                   top_offs + (option.rect.height() - datacenterPixmap->height()) / 2, painter);
        }

        // static ip text
        painter->setOpacity(0.5);
        IndependentPixmap pixmapStaticIp = cache->pixmap(CityItemDelegateCache::kStaticIpId);
        Q_ASSERT(!pixmapStaticIp.isNull());
        QRect rc( option.rect.width() - pixmapStaticIp.width() - 44*G_SCALE,  option.rect.top(), pixmapStaticIp.width(), option.rect.height());
        pixmapStaticIp.draw(rc.left(), rc.top() + (rc.height() -  pixmapStaticIp.height()) / 2, painter);
    }
    else if (index.data(kIsShowAsPremium).toBool())
    {
        // pro star
        QSharedPointer<IndependentPixmap> premiumStarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_CITY_STAR");
        premiumStarPixmap->draw(left_offs + LOCATION_ITEM_MARGIN * G_SCALE,
                                top_offs + (option.rect.height() - premiumStarPixmap->height()) / 2, painter);
    }
    else if (lid.isCustomConfigsLocation())
    {
        QString type = index.data(kCustomConfigType).toString();
        if (!type.isEmpty())
        {
            QSharedPointer<IndependentPixmap> configPixmap = ImageResourcesSvg::instance().getIndependentPixmap(
                QString("locations/%1_ICON").arg(type.toUpper()));
            if (configPixmap) {
                painter->setOpacity(textOpacity);
                configPixmap->draw(left_offs + LOCATION_ITEM_MARGIN * G_SCALE,
                                   top_offs + (option.rect.height() - configPixmap->height()) / 2, painter);
            }
        }

    }
    else // fav
    {
        QSharedPointer<IndependentPixmap> favIcon;
        if (index.data(kIsFavorite).toBool() == true) {
            favIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON_SELECTED");
        }
        else  {
            favIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON_DESELECTED");
        }
        painter->setOpacity(OPACITY_HALF);
        favIcon->draw(left_offs + 24*G_SCALE, top_offs + (option.rect.height() - favIcon->height()) / 2, painter);
    }

    // text
    painter->setOpacity(textOpacity);
    IndependentPixmap pixmapCaption = cache->pixmap(CityItemDelegateCache::kCityId);
    Q_ASSERT(!pixmapCaption.isNull());
    QRect rcCaption( left_offs + LOCATION_ITEM_MARGIN * G_SCALE * 2 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE,  option.rect.top(), pixmapCaption.width(), option.rect.height());
    pixmapCaption.draw(rcCaption.left(), rcCaption.top() + (rcCaption.height() -  pixmapCaption.height()) / 2, painter);

    // city text for non-static and non-custom views only
    if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation())
    {
        IndependentPixmap pixmapNick = cache->pixmap(CityItemDelegateCache::kNickId);
        Q_ASSERT(!pixmapNick.isNull());
        QRect rc( rcCaption.left() + rcCaption.width() +  8*G_SCALE,  option.rect.top(), pixmapNick.width(), option.rect.height());
        pixmapNick.draw(rc.left(), rc.top() + (rc.height() -  pixmapNick.height()) / 2, painter);
    }

    // only show disabled locations to pro users
    bool disabled = index.data(kIsShowAsPremium).toBool() ? false : index.data(kIsDisabled).toBool();
    if (disabled)
    {
        QSharedPointer<IndependentPixmap> consIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/UNDER_CONSTRUCTION_ICON");
        painter->setOpacity(OPACITY_HALF);
        consIcon->draw(left_offs + option.rect.width() - LOCATION_ITEM_MARGIN*G_SCALE - consIcon->width(),
                       top_offs + (option.rect.height() - consIcon->height()) / 2,
                       painter);
    }
    // is broken custom config?
    else if (lid.isCustomConfigsLocation() && !index.data(kIsCustomConfigCorrect).toBool())
    {
        QSharedPointer<IndependentPixmap> errorIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/ERROR_ICON");
        errorIcon->draw(left_offs + option.rect.width() - errorIcon->width() - LOCATION_ITEM_MARGIN * G_SCALE,
                        top_offs + (option.rect.height() - errorIcon->height()) / 2  - 1*G_SCALE,
                        painter);
    }
    else if (option.isShowLatencyInMs())
    {
        // draw bubble
        int latencyRectWidth = 33;
        int latencyRectHeight = 20;
        int scaledX = option.rect.width() - (latencyRectWidth + 8) * G_SCALE;
        int scaledY = (option.rect.height() - (latencyRectHeight * G_SCALE)) / 2 - 1*G_SCALE;
        QRect latencyRect(left_offs + scaledX, top_offs + scaledY,
                          latencyRectWidth * G_SCALE, latencyRectHeight *G_SCALE);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(QBrush(QColor(40, 45, 61)));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(latencyRect, 4*G_SCALE, 4*G_SCALE);

        // draw latency text
        QFont font = *FontManager::instance().getFont(11, false);
        QString latencyText = QString::number(index.data(kPingTime).toInt());
        painter->setBrush(Qt::white);
        painter->setPen(Qt::white);
        painter->setFont(font);
        painter->drawText(QRect((left_offs + scaledX + latencyRectWidth/2*G_SCALE) - CommonGraphics::textWidth(latencyText, font)/2,
                               (top_offs + scaledY + latencyRectHeight/2*G_SCALE) - CommonGraphics::textHeight(font)/2 ,
                               CommonGraphics::textWidth(latencyText, font),
                               CommonGraphics::textHeight(font)),
                         latencyText);
    }
    // show ping icon
    else
    {
        PingTime pingTime = index.data(kPingTime).toInt();
        QSharedPointer<IndependentPixmap> pingIcon = ImageResourcesSvg::instance().getIndependentPixmap(pingIconNameString(pingTime.toConnectionSpeed()));
        int scaledX = option.rect.width() - pingIcon->width() - LOCATION_ITEM_MARGIN * G_SCALE;
        int scaledY = (option.rect.height() - pingIcon->height()) / 2 - 1*G_SCALE;
        painter->setOpacity(OPACITY_FULL);
        pingIcon->draw(left_offs + scaledX, top_offs + scaledY, painter);
    }

    // 10gbps icon
    if (index.data(kIs10Gbps).toBool())
    {
        painter->setOpacity(OPACITY_FULL);
        QSharedPointer<IndependentPixmap> tenGbpsPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/10_GBPS_ICON");
        tenGbpsPixmap->draw(left_offs + option.rect.width() - LOCATION_ITEM_MARGIN*G_SCALE - tenGbpsPixmap->width() - 32 * G_SCALE, top_offs + (option.rect.height() - tenGbpsPixmap->height()) / 2, painter);
    }

    // background line
    // TODO: lines drawn like this do not scale -- fix
    int left = left_offs + static_cast<int>(24 * G_SCALE);
    int right = left_offs + static_cast<int>(option.rect.width() - 8*G_SCALE);
    int bottom = top_offs + option.rect.height() - 1; // 1 is not scaled since we want bottom-most pixel inside geometry
    QPen pen(QColor(0x29, 0x2E, 0x3E));
    pen.setWidth(1);
    painter->setOpacity(initOpacity);
    painter->setPen(pen);
    painter->drawLine(left, bottom - 1, right, bottom - 1);
    painter->drawLine(left, bottom, right, bottom);

    if (option.isShowLocationLoad())
    {
        int locationLoad = index.data(kLoad).toInt();
        if (locationLoad > 0)
        {
            Qt::GlobalColor penColor;
            if (locationLoad < 60) {
                penColor = Qt::green;
            }
            else if (locationLoad < 90) {
                penColor = Qt::yellow;
            }
            else {
                penColor = Qt::red;
            }
            int rightX = left + ((right - left) * locationLoad / 100);
            QPen penLoad(penColor);
            penLoad.setWidth(1);
            painter->setOpacity(textOpacity);
            painter->setPen(penLoad);
            painter->drawLine(left, bottom - 1, rightX, bottom - 1);
            painter->drawLine(left, bottom, rightX, bottom);
            painter->setOpacity(1.0);
        }
    }

    painter->restore();
}

IItemCacheData *CityItemDelegate::createCacheData(const QModelIndex &index) const
{
    CityItemDelegateCache *cache = new CityItemDelegateCache();

    QString cityCaption = CommonGraphics::maybeTruncatedText(index.data(kName).toString(),
                                                              *FontManager::instance().getFont(16, true),
                                                              static_cast<int>(CITY_CAPTION_MAX_WIDTH * G_SCALE));
    cache->add(CityItemDelegateCache::kCityId, cityCaption, *FontManager::instance().getFont(16, true), DpiScaleManager::instance().curDevicePixelRatio());
    cache->add(CityItemDelegateCache::kNickId, index.data(kNick).toString(), *FontManager::instance().getFont(16, false), DpiScaleManager::instance().curDevicePixelRatio());
    cache->add(CityItemDelegateCache::kStaticIpId, index.data(kStaticIp).toString(), *FontManager::instance().getFont(13, false), DpiScaleManager::instance().curDevicePixelRatio());
    return cache;
}

void CityItemDelegate::updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const
{
    CityItemDelegateCache *cache = static_cast<CityItemDelegateCache *>(cacheData);
    QString cityCaption = CommonGraphics::maybeTruncatedText(index.data(kName).toString(),
                                                              *FontManager::instance().getFont(16, true),
                                                              static_cast<int>(CITY_CAPTION_MAX_WIDTH * G_SCALE));
    cache->updateIfTextChanged(CityItemDelegateCache::kCityId, cityCaption, *FontManager::instance().getFont(16, true), DpiScaleManager::instance().curDevicePixelRatio());
    cache->updateIfTextChanged(CityItemDelegateCache::kNickId, index.data(kNick).toString(), *FontManager::instance().getFont(16, false), DpiScaleManager::instance().curDevicePixelRatio());
    cache->updateIfTextChanged(CityItemDelegateCache::kStaticIpId, index.data(kStaticIp).toString(), *FontManager::instance().getFont(13, false), DpiScaleManager::instance().curDevicePixelRatio());
}

bool CityItemDelegate::isForbiddenCursor(const QModelIndex &index) const
{
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (lid.isCustomConfigsLocation() && !index.data(kIsCustomConfigCorrect).toBool())
    {
        return true;
    }
    else
    {
        return index.data(kIsShowAsPremium).toBool() ? false : index.data(kIsDisabled).toBool();
    }
}

int CityItemDelegate::isInClickableArea(const QModelIndex &index, const QPoint &point, const QRect &itemRect) const
{
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (lid.isStaticIpsLocation() || lid.isCustomConfigsLocation()) {
        return -1;
    }

    QSharedPointer<IndependentPixmap> favIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON_SELECTED");
    QRect rc(24*G_SCALE, (itemRect.height() - favIcon->height()) / 2, favIcon->width(), favIcon->height());
    if (rc.contains(point))
    {
        return CLICKABLE_FAVORITE_RECT;
    }
    return -1;
}

int CityItemDelegate::isInTooltipArea(const QModelIndex &index, const QPoint &point) const
{
    return -1;
}

QString CityItemDelegate::pingIconNameString(int connectionSpeedIndex) const
{
    if (connectionSpeedIndex == 0)
    {
        return "locations/LOCATION_PING_BARS0";
    }
    else if (connectionSpeedIndex == 1)
    {
        return "locations/LOCATION_PING_BARS1";
    }
    else if (connectionSpeedIndex == 2)
    {
        return "locations/LOCATION_PING_BARS2";
    }
    else
    {
        return "locations/LOCATION_PING_BARS3";
    }
    Q_ASSERT(false);
}

} // namespace gui_locations
