#include "cityitemdelegate.h"

#include <QPainter>
#include <QtMath>

#include "locations/locationsmodel_roles.h"
#include "clickableandtooltiprects.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "types/locationid.h"
#include "types/pingtime.h"
#include "textpixmap.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"

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
    const int left_offs = option.rect.left();
    const int top_offs = option.rect.top();
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
            WS_ASSERT(false);
        }

        if (datacenterPixmap)
        {
            datacenterPixmap->draw(left_offs + LOCATION_ITEM_MARGIN_TO_LINE * G_SCALE,
                                   top_offs + (option.rect.height() - datacenterPixmap->height()) / 2, painter);
        }

        // static ip text
        painter->setOpacity(0.5);
        IndependentPixmap pixmapStaticIp = cache->pixmap(CityItemDelegateCache::kStaticIpId);
        WS_ASSERT(!pixmapStaticIp.isNull());
        QRect rc( option.rect.width() - pixmapStaticIp.width() - 38*G_SCALE,  option.rect.top(), pixmapStaticIp.width(), option.rect.height());
        pixmapStaticIp.draw(rc.left(), rc.top() + (rc.height() -  pixmapStaticIp.height()) / 2, painter);
    }
    else if (index.data(kIsShowAsPremium).toBool())
    {
        // pro star
        QSharedPointer<IndependentPixmap> premiumStarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_CITY_STAR");
        premiumStarPixmap->draw(left_offs + LOCATION_ITEM_MARGIN * 2 * G_SCALE,
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
                configPixmap->draw(left_offs + LOCATION_ITEM_MARGIN * 2 * G_SCALE,
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
    WS_ASSERT(!pixmapCaption.isNull());
    QRect rcCaption( left_offs + LOCATION_ITEM_MARGIN * G_SCALE * 4 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE,  option.rect.top(), pixmapCaption.width(), option.rect.height());
    pixmapCaption.draw(rcCaption.left(), rcCaption.top() + (rcCaption.height() -  pixmapCaption.height()) / 2, painter);

    // city text for non-static and non-custom views only
    if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation())
    {
        IndependentPixmap pixmapNick = cache->pixmap(CityItemDelegateCache::kNickId);
        WS_ASSERT(!pixmapNick.isNull());
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
        int scaledX = option.rect.width() - ((latencyRectWidth + 2) * G_SCALE);
        int scaledY = (option.rect.height() - (latencyRectHeight * G_SCALE)) / 2 - 1*G_SCALE;
        QRect latencyRect(left_offs + scaledX, top_offs + scaledY,
                          latencyRectWidth * G_SCALE, latencyRectHeight *G_SCALE);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(QBrush(QColor(40, 45, 61)));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(latencyRect, 4*G_SCALE, 4*G_SCALE);

        // draw latency text
        QFont font = FontManager::instance().getFont(11, false);
        int latency = index.data(kPingTime).toInt();
        QString latencyText = (latency <= 0 ? "--" : QString::number(latency));
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
    int right = left_offs + static_cast<int>(option.rect.width() - 2*G_SCALE);
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
    bool is10Gbps = index.data(kIs10Gbps).toBool();
    int maxWidth = is10Gbps ? (kCityItemMaxWidth - 32)*G_SCALE : kCityItemMaxWidth*G_SCALE;

    QString cityCaption = CommonGraphics::maybeTruncatedText(index.data().toString(),
                                                             FontManager::instance().getFont(16, true),
                                                             static_cast<int>(kCityCaptionMaxWidth*G_SCALE));
    cache->add(CityItemDelegateCache::kCityId, cityCaption, FontManager::instance().getFont(16, true), DpiScaleManager::instance().curDevicePixelRatio());
    QString nickCaption = CommonGraphics::maybeTruncatedText(index.data(kDisplayNickname).toString(),
                                                             FontManager::instance().getFont(16, false),
                                                             static_cast<int>(maxWidth) - 8*G_SCALE - cache->pixmap(CityItemDelegateCache::kCityId).width());
    cache->add(CityItemDelegateCache::kNickId, nickCaption, FontManager::instance().getFont(16, false), DpiScaleManager::instance().curDevicePixelRatio());
    cache->add(CityItemDelegateCache::kStaticIpId, index.data(kStaticIp).toString(), FontManager::instance().getFont(13, false), DpiScaleManager::instance().curDevicePixelRatio());
    return cache;
}

void CityItemDelegate::updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const
{
    CityItemDelegateCache *cache = static_cast<CityItemDelegateCache *>(cacheData);
    bool is10Gbps = index.data(kIs10Gbps).toBool();
    int maxWidth = is10Gbps ? (kCityItemMaxWidth - 32)*G_SCALE : kCityItemMaxWidth*G_SCALE;

    QString cityCaption = CommonGraphics::maybeTruncatedText(index.data().toString(),
                                                              FontManager::instance().getFont(16, true),
                                                              static_cast<int>(kCityCaptionMaxWidth*G_SCALE));
    cache->updateIfTextChanged(CityItemDelegateCache::kCityId, cityCaption, FontManager::instance().getFont(16, true), DpiScaleManager::instance().curDevicePixelRatio());
    QString nickCaption = CommonGraphics::maybeTruncatedText(index.data(kDisplayNickname).toString(),
                                                             FontManager::instance().getFont(16, false),
                                                             static_cast<int>(maxWidth) - 8*G_SCALE - cache->pixmap(CityItemDelegateCache::kCityId).width());
    cache->updateIfTextChanged(CityItemDelegateCache::kNickId, nickCaption, FontManager::instance().getFont(16, false), DpiScaleManager::instance().curDevicePixelRatio());
    cache->updateIfTextChanged(CityItemDelegateCache::kStaticIpId, index.data(kStaticIp).toString(), FontManager::instance().getFont(13, false), DpiScaleManager::instance().curDevicePixelRatio());
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
        return index.data(kIsShowAsPremium).toBool() ? true : index.data(kIsDisabled).toBool();
    }
}

int CityItemDelegate::isInClickableArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point) const
{
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (lid.isStaticIpsLocation() || lid.isCustomConfigsLocation()) {
        return -1;
    }
    if (index.data(kIsShowAsPremium).toBool()) {
        return -1;
    }

    QSharedPointer<IndependentPixmap> favIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON_SELECTED");
    QRect rc(24*G_SCALE + option.rect.left(), (option.rect.height() - favIcon->height()) / 2 + option.rect.top(), favIcon->width(), favIcon->height());
    rc.adjust(-2*G_SCALE, -2*G_SCALE, 2*G_SCALE, 2*G_SCALE);    //add a little more area for the convenience of clicking
    if (rc.contains(point))
    {
        return (int)ClickableRect::kFavorite;
    }
    return (int)ClickableRect::kNone;
}

int CityItemDelegate::isInTooltipArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point, const IItemCacheData *cacheData) const
{
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));

    // is broken custom config?
    if (lid.isCustomConfigsLocation() && !index.data(kIsCustomConfigCorrect).toBool()) {
        if (latencyIconRect(option.rect).contains(point)) {
            return (int)TooltipRect::kCustomConfigErrorMessage;
        }
    } else if (!option.isShowLatencyInMs())
    {
        if (latencyIconRect(option.rect).contains(point)) {
            return (int)TooltipRect::kPingTime;
        }
    }

    if (captionRect(option.rect, cacheData).contains(point)) {
        QString originalCaption = index.data().toString();
        QString truncatedCaption = CommonGraphics::maybeTruncatedText(originalCaption,
                                                                      FontManager::instance().getFont(16, true),
                                                                      static_cast<int>(kCityCaptionMaxWidth*G_SCALE));
        if (originalCaption != truncatedCaption)
            return (int)TooltipRect::kItemCaption;
    }

    // Check if nickname is elided and mouse is over it
    if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation()) {
        if (nicknameRect(option.rect, cacheData).contains(point)) {
            QString originalNickname = index.data(kDisplayNickname).toString();
            bool is10Gbps = index.data(kIs10Gbps).toBool();
            int maxWidth = is10Gbps ? (kCityItemMaxWidth - 32)*G_SCALE : kCityItemMaxWidth*G_SCALE;

            const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);
            QString truncatedNickname = CommonGraphics::maybeTruncatedText(originalNickname,
                                                                           FontManager::instance().getFont(16, false),
                                                                           static_cast<int>(maxWidth) - 8*G_SCALE - cache->pixmap(CityItemDelegateCache::kCityId).width());
            if (originalNickname != truncatedNickname)
                return (int)TooltipRect::kItemNickname;
        }
    }

    return (int)TooltipRect::kNone;
}

void CityItemDelegate::tooltipEnterEvent(const ItemStyleOption &option, const QModelIndex &index, int tooltipId, const IItemCacheData *cacheData) const
{
    WS_ASSERT(dynamic_cast<QWidget *>(option.styleObject) != nullptr);
    QWidget *widget = static_cast<QWidget *>(option.styleObject);
    if (tooltipId == (int)TooltipRect::kCustomConfigErrorMessage) {
        QString text = index.data(gui_locations::kCustomConfigErrorMessage).toString();
        if (text.isEmpty())
            text = QWidget::tr("Unknown Config Error");

        QRect rc = latencyIconRect(option.rect);
        QPoint pt = widget->mapToGlobal(QPoint(rc.center().x(), rc.top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = text;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.8;
        TooltipController::instance().showTooltipBasic(ti);

    } else if (tooltipId == (int)TooltipRect::kPingTime) {
        QRect rc = latencyIconRect(option.rect);
        QPoint pt = widget->mapToGlobal(QPoint(rc.center().x(), rc.top() - 3*G_SCALE));
        int pingTime = index.data(kPingTime).toInt();
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_PING_TIME);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = QString("%1 ms").arg(pingTime);
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    } else if (tooltipId == (int)TooltipRect::kItemCaption) {
        QRect rc = captionRect(option.rect, cacheData);
        QPoint pt = widget->mapToGlobal(QPoint(rc.x() + rc.width()/2, rc.top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = index.data().toString();
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    } else if (tooltipId == (int)TooltipRect::kItemNickname) {
        QRect rc = nicknameRect(option.rect, cacheData);
        QPoint pt = widget->mapToGlobal(QPoint(rc.x() + rc.width()/2, rc.top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = index.data(kDisplayNickname).toString();
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    } else
    {
        WS_ASSERT(false);
    }
}

void CityItemDelegate::tooltipLeaveEvent(int tooltipId) const
{
    if (tooltipId == (int)TooltipRect::kCustomConfigErrorMessage)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
    else if (tooltipId == (int)TooltipRect::kPingTime)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
    else if (tooltipId == (int)TooltipRect::kItemCaption)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
    else if (tooltipId == (int)TooltipRect::kItemNickname)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    else
        WS_ASSERT(false);
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
    WS_ASSERT(false);
}

QRect CityItemDelegate::latencyIconRect(const QRect &itemRect) const
{
    QSharedPointer<IndependentPixmap> pingIcon = ImageResourcesSvg::instance().getIndependentPixmap(pingIconNameString(0));
    int scaledX = itemRect.width() - pingIcon->width() - LOCATION_ITEM_MARGIN * G_SCALE;
    int scaledY = (itemRect.height() - pingIcon->height()) / 2 - 1*G_SCALE;
    return QRect(itemRect.left() + scaledX, itemRect.top() + scaledY,
                 pingIcon->width(), pingIcon->height());
}

QRect CityItemDelegate::captionRect(const QRect &itemRect, const IItemCacheData *cacheData) const
{
    const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);
    IndependentPixmap pixmapCaption = cache->pixmap(CityItemDelegateCache::kCityId);
    return QRect(itemRect.left() + LOCATION_ITEM_MARGIN * G_SCALE * 4 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE,
                 itemRect.top() + (itemRect.height() - pixmapCaption.height()) / 2,
                 pixmapCaption.width(), pixmapCaption.height());
}

QRect CityItemDelegate::nicknameRect(const QRect &itemRect, const IItemCacheData *cacheData) const
{
    const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);
    IndependentPixmap pixmapCaption = cache->pixmap(CityItemDelegateCache::kCityId);
    IndependentPixmap pixmapNick = cache->pixmap(CityItemDelegateCache::kNickId);

    QRect rcCaption(itemRect.left() + LOCATION_ITEM_MARGIN * G_SCALE * 4 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE,
                   itemRect.top() + (itemRect.height() - pixmapCaption.height()) / 2,
                   pixmapCaption.width(), pixmapCaption.height());

    return QRect(rcCaption.left() + rcCaption.width() + 8*G_SCALE,
                itemRect.top() + (itemRect.height() - pixmapNick.height()) / 2,
                pixmapNick.width(), pixmapNick.height());
}

} // namespace gui_locations
