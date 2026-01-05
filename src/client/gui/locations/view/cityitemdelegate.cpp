#include "cityitemdelegate.h"

#include <QObject>
#include <QPainter>
#include <QPainterPath>
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
          kStaticIpId,
          kPinnedIpId
    };
};
}

namespace gui_locations {

void CityItemDelegate::paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const
{
    painter->save();

    const int left_offs = option.rect.left();
    const int top_offs = option.rect.top();
    double textOpacity = OPACITY_SEVENTY + (OPACITY_FULL - OPACITY_SEVENTY) * option.selectedOpacity();

    const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);

    // background
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(OPACITY_FULL);
    painter->fillRect(option.rect, FontManager::instance().getMidnightColor());
    if (qFuzzyCompare(option.selectedOpacity(), 1.0)) {
        painter->fillRect(option.rect, QColor(255, 255, 255, 13));
    }

    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    QRect leftRect = QRect(left_offs + 16*G_SCALE, top_offs + (option.rect.height() - 24*G_SCALE) / 2, 24*G_SCALE, 24*G_SCALE);
    if (lid.isStaticIpsLocation()) {
        QString staticIpType = index.data(kStaticIpType).toString();
        // static ip icon
        QSharedPointer<IndependentPixmap> datacenterPixmap;
        if (staticIpType.compare("dc", Qt::CaseInsensitive) == 0) {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/DATACENTER_ICON");
        } else if (staticIpType.compare("res", Qt::CaseInsensitive) == 0) {
            datacenterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/RES_IP_ICON");
        } else {
            WS_ASSERT(false);
        }

        if (datacenterPixmap) {
            datacenterPixmap->draw(leftRect, painter);
        }
    } else if (index.data(kIsShowAsPremium).toBool()) {
        // pro star
        QSharedPointer<IndependentPixmap> premiumStarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_CITY_STAR");
        premiumStarPixmap->draw(leftRect, painter);
    } else if (lid.isCustomConfigsLocation()) {
        QString type = index.data(kCustomConfigType).toString();
        if (!type.isEmpty()) {
            QSharedPointer<IndependentPixmap> configPixmap = ImageResourcesSvg::instance().getIndependentPixmap(
                QString("locations/%1_ICON").arg(type.toUpper()));
            if (configPixmap) {
                configPixmap->draw(left_offs + 16*G_SCALE,
                                   top_offs + (option.rect.height() - configPixmap->height()) / 2, painter);
            }
        }
    } else {
        if (option.isShowCountryFlagForCity()) {
            QSharedPointer<IndependentPixmap> flag = ImageResourcesSvg::instance().getCircleFlag(index.data(kCountryCode).toString());
            flag->draw(leftRect, painter);
        } else if (index.data(kIs10Gbps).toBool()) {
            // 10gbps icon
            QSharedPointer<IndependentPixmap> tenGbpsPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/10_GBPS_ICON");
            QRect iconRect = QRect(left_offs + 18*G_SCALE, top_offs + (option.rect.height() - 20*G_SCALE) / 2, 20*G_SCALE, 20*G_SCALE);
            tenGbpsPixmap->draw(iconRect, painter);
        } else { // generic city icon
            QSharedPointer<IndependentPixmap> cityIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/CITY_ICON");
            QRect iconRect = QRect(left_offs + 18*G_SCALE, top_offs + (option.rect.height() - 20*G_SCALE) / 2, 20*G_SCALE, 20*G_SCALE);
            cityIcon->draw(iconRect, painter);
        }

        // Load
        if (option.isShowLocationLoad()) {
            painter->setOpacity(OPACITY_FULL);
            // draw a 1px 'border' around the icon to differentiate from the load circle
            QPen penBorder(QColor(9, 15, 25));
            penBorder.setWidth(G_SCALE);
            painter->setPen(penBorder);
            painter->drawArc(leftRect.adjusted(G_SCALE, G_SCALE, -G_SCALE, -G_SCALE), 0, 360 * 16);

            // Draw a full circle with grey color: this is (255, 255, 255, 51) blended with (9, 15, 25).
            // Drawing (255, 255, 255, 51) is not visible since it draws on top of some transparent pixels.
            QPen penLoad(QColor(53, 61, 73));
            penLoad.setWidth(2*G_SCALE);
            penLoad.setColor(QColor(53, 61, 73));
            painter->setPen(penLoad);
            painter->drawArc(leftRect, 0, 360 * 16);
            int locationLoad = index.data(kLoad).toInt();
            if (locationLoad > 0) {
                if (locationLoad < 10) {
                    locationLoad = 10; // Minimum value is 10, otherwise the arc is too short
                }
                Qt::GlobalColor penColor;
                if (locationLoad < 60) {
                    penColor = Qt::green;
                } else if (locationLoad < 90) {
                    penColor = Qt::yellow;
                } else {
                    penColor = Qt::red;
                }
                // Draw arc with pen color
                penLoad.setColor(penColor);
                painter->setPen(penLoad);
                painter->drawArc(leftRect, 90 * 16, -360 * locationLoad * 16 / 100);
            }
        }
    }

    // text
    painter->setOpacity(textOpacity);
    IndependentPixmap pixmapCaption = cache->pixmap(CityItemDelegateCache::kCityId);
    WS_ASSERT(!pixmapCaption.isNull());

    // In favorites tab, show city name higher and pinned IP below
    if (option.isShowCountryFlagForCity() || lid.isStaticIpsLocation()) {
        QRect rcCaption(left_offs + 56*G_SCALE,  option.rect.top() + 2*G_SCALE, pixmapCaption.width(), pixmapCaption.height());
        pixmapCaption.draw(rcCaption.left(), rcCaption.top(), painter);

        if (option.isShowCountryFlagForCity()) {
            IndependentPixmap pixmapNick = cache->pixmap(CityItemDelegateCache::kNickId);
            WS_ASSERT(!pixmapNick.isNull());
            QRect rc(rcCaption.left() + rcCaption.width() + 8*G_SCALE, option.rect.top() + 2*G_SCALE, pixmapNick.width(), pixmapNick.height());
            pixmapNick.draw(rc.left(), rc.top(), painter);
            painter->setOpacity(0.6);

            IndependentPixmap pixmapPinnedIp = cache->pixmap(CityItemDelegateCache::kPinnedIpId);
            WS_ASSERT(!pixmapPinnedIp.isNull());
            pixmapPinnedIp.draw(left_offs + 56*G_SCALE, option.rect.top() + 2*G_SCALE + pixmapCaption.height(), painter);
        } else {
            IndependentPixmap pixmapStaticIp = cache->pixmap(CityItemDelegateCache::kStaticIpId);
            WS_ASSERT(!pixmapStaticIp.isNull());
            pixmapStaticIp.draw(left_offs + 56*G_SCALE, option.rect.top() + 2*G_SCALE + pixmapCaption.height(), painter);
        }
    } else {
        QRect rcCaption(left_offs + 56*G_SCALE,  option.rect.top(), pixmapCaption.width(), option.rect.height());
        pixmapCaption.draw(rcCaption.left(), rcCaption.top() + (rcCaption.height() -  pixmapCaption.height()) / 2, painter);

        // city text for non-static and non-custom views only (not in favorites tab)
        if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation()) {
            IndependentPixmap pixmapNick = cache->pixmap(CityItemDelegateCache::kNickId);
            WS_ASSERT(!pixmapNick.isNull());
            QRect rc( rcCaption.left() + rcCaption.width() + 8*G_SCALE,  option.rect.top(), pixmapNick.width(), option.rect.height());
            pixmapNick.draw(rc.left(), rc.top() + (rc.height() -  pixmapNick.height()) / 2, painter);
        }
    }

    // The rest are aligned to the right.  We start drawing from the right-most icon, and work our way left.
    int xOffset = left_offs + option.rect.width() - 27*G_SCALE;

    // Disabled/custom config error/latency

    // only show disabled locations to pro users
    bool disabled = index.data(kIsShowAsPremium).toBool() ? false : index.data(kIsDisabled).toBool();
    if (disabled) {
        QSharedPointer<IndependentPixmap> consIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/UNDER_CONSTRUCTION_ICON");
        painter->setOpacity(textOpacity);
        xOffset -= consIcon->width();
        consIcon->draw(xOffset, top_offs + (option.rect.height() - consIcon->height()) / 2, painter);
        xOffset -= LOCATION_ITEM_MARGIN*G_SCALE;
    } else if (lid.isCustomConfigsLocation() && !index.data(kIsCustomConfigCorrect).toBool()) { // is broken custom config?
        bool isHoveringError = (option.hoverClickableId() == (int)ClickableRect::kErrorIcon);
        double errorOpacity = 0.4;
        if (qFuzzyCompare(option.selectedOpacity(), 1.0)) {
            errorOpacity = isHoveringError ? 1.0 : OPACITY_SEVENTY;
        }
        painter->setOpacity(errorOpacity);
        QSharedPointer<IndependentPixmap> errorIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/ERROR_ICON");
        xOffset -= errorIcon->width();
        errorIcon->draw(xOffset, top_offs + (option.rect.height() - errorIcon->height()) / 2  - 1*G_SCALE, painter);
        xOffset -= LOCATION_ITEM_MARGIN*G_SCALE;
    } else {
        // Draw ping icon
        PingTime pingTime = index.data(kPingTime).toInt();
        QSharedPointer<IndependentPixmap> pingIcon = ImageResourcesSvg::instance().getIndependentPixmap(pingIconNameString(pingTime.toConnectionSpeed()));
        painter->setOpacity(OPACITY_FULL);
        xOffset -= 16*G_SCALE;
        pingIcon->draw(xOffset + (16*G_SCALE - pingIcon->width())/2, top_offs + 6*G_SCALE, painter);

        QFont font = FontManager::instance().getFont(9, QFont::Light);
        int latency = index.data(kPingTime).toInt();
        QString latencyText = (latency <= 0 ? "--" : QString::number(latency));

        // latency text box
        QPainterPath path;
        path.addRoundedRect(QRect(xOffset + (16*G_SCALE - CommonGraphics::textWidth(latencyText, font))/2 - 3*G_SCALE,
                                  top_offs + 21*G_SCALE,
                                  CommonGraphics::textWidth(latencyText, font) + 6*G_SCALE,
                                  CommonGraphics::textHeight(font) + 2*G_SCALE),
                            3*G_SCALE, 3*G_SCALE);
        painter->setPen(Qt::NoPen);
        painter->fillPath(path, QColor(255, 255, 255, 26));
        painter->setPen(Qt::SolidLine);

        // draw latency text
        painter->setBrush(Qt::white);
        painter->setPen(Qt::white);
        painter->setFont(font);
        painter->drawText(QRect(xOffset + (16*G_SCALE - CommonGraphics::textWidth(latencyText, font))/2,
                                top_offs + 22*G_SCALE,
                                CommonGraphics::textWidth(latencyText, font),
                                CommonGraphics::textHeight(font)),
                          latencyText);
        xOffset -= LOCATION_ITEM_MARGIN*G_SCALE;
    }

    // favorite icon
    // You can't favorite a static ip or custom config location.
    if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation()) {
        QSharedPointer<IndependentPixmap> favIcon;
        bool isHoveringFavorite = (option.hoverClickableId() == (int)ClickableRect::kFavorite);
        if (qFuzzyCompare(option.selectedOpacity(), 1.0)) {
            if (index.data(kIsFavorite).toBool() == true) {
                painter->setOpacity(isHoveringFavorite ? OPACITY_FULL : OPACITY_SEVENTY);
                favIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON_SELECTED");
                favIcon->draw(xOffset - favIcon->width(), top_offs + (option.rect.height() - favIcon->height()) / 2, painter);
            } else {
                painter->setOpacity(isHoveringFavorite ? OPACITY_FULL : OPACITY_SEVENTY);
                favIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON_DESELECTED");
                favIcon->draw(xOffset - favIcon->width(), top_offs + (option.rect.height() - favIcon->height()) / 2, painter);
            }
        }
    }

    painter->restore();
}

IItemCacheData *CityItemDelegate::createCacheData(const QModelIndex &index) const
{
    CityItemDelegateCache *cache = new CityItemDelegateCache();
    int maxWidth = kCityItemMaxWidth;
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));

    QString cityCaption = CommonGraphics::maybeTruncatedText(index.data().toString(),
                                                             FontManager::instance().getFont(15, QFont::Medium),
                                                             (lid.isCustomConfigsLocation() ? maxWidth : kCityCaptionMaxWidth)*G_SCALE);
    cache->add(CityItemDelegateCache::kCityId, cityCaption, FontManager::instance().getFont(15, QFont::Medium), DpiScaleManager::instance().curDevicePixelRatio());

    if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation()) {
        QString nickCaption = CommonGraphics::maybeTruncatedText(index.data(kDisplayNickname).toString(),
                                                                 FontManager::instance().getFont(15, QFont::Normal),
                                                                 (maxWidth - 8)*G_SCALE - cache->pixmap(CityItemDelegateCache::kCityId).width());
        cache->add(CityItemDelegateCache::kNickId, nickCaption, FontManager::instance().getFont(15, QFont::Normal), DpiScaleManager::instance().curDevicePixelRatio());
    }
    cache->add(CityItemDelegateCache::kStaticIpId, index.data(kStaticIp).toString(), FontManager::instance().getFont(12, QFont::Normal), DpiScaleManager::instance().curDevicePixelRatio());

    QVariantList pinnedData = index.data(kPinnedIp).toList();
    QString pinnedIp = (pinnedData.size() == 2) ? pinnedData[1].toString() : QString();
    QString ipText = pinnedIp.isEmpty() ? QObject::tr("Random IP") : pinnedIp;
    cache->add(CityItemDelegateCache::kPinnedIpId, ipText, FontManager::instance().getFont(12, QFont::Normal), DpiScaleManager::instance().curDevicePixelRatio());

    return cache;
}

void CityItemDelegate::updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const
{
    CityItemDelegateCache *cache = static_cast<CityItemDelegateCache *>(cacheData);
    int maxWidth = kCityItemMaxWidth;
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));

    QString cityCaption = CommonGraphics::maybeTruncatedText(index.data().toString(),
                                                              FontManager::instance().getFont(15, QFont::Medium),
                                                              (lid.isCustomConfigsLocation() ? maxWidth : kCityCaptionMaxWidth)*G_SCALE);
    cache->updateIfTextChanged(CityItemDelegateCache::kCityId, cityCaption, FontManager::instance().getFont(15, QFont::Medium), DpiScaleManager::instance().curDevicePixelRatio());

    if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation()) {
        QString nickCaption = CommonGraphics::maybeTruncatedText(index.data(kDisplayNickname).toString(),
                                                                 FontManager::instance().getFont(15, QFont::Normal),
                                                                 (maxWidth - 8)*G_SCALE - cache->pixmap(CityItemDelegateCache::kCityId).width());
        cache->updateIfTextChanged(CityItemDelegateCache::kNickId, nickCaption, FontManager::instance().getFont(15, QFont::Normal), DpiScaleManager::instance().curDevicePixelRatio());
    }
    cache->updateIfTextChanged(CityItemDelegateCache::kStaticIpId, index.data(kStaticIp).toString(), FontManager::instance().getFont(12, QFont::Normal), DpiScaleManager::instance().curDevicePixelRatio());

    QVariantList pinnedData = index.data(kPinnedIp).toList();
    QString pinnedIp = (pinnedData.size() == 2) ? pinnedData[1].toString() : QString();
    QString ipText = pinnedIp.isEmpty() ? QObject::tr("Random IP") : pinnedIp;
    cache->updateIfTextChanged(CityItemDelegateCache::kPinnedIpId, ipText, FontManager::instance().getFont(12, QFont::Normal), DpiScaleManager::instance().curDevicePixelRatio());
}

bool CityItemDelegate::isForbiddenCursor(const QModelIndex &index) const
{
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (lid.isCustomConfigsLocation() && !index.data(kIsCustomConfigCorrect).toBool()) {
        return true;
    } else {
        return index.data(kIsShowAsPremium).toBool() ? true : index.data(kIsDisabled).toBool();
    }
}

int CityItemDelegate::isInClickableArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point) const
{
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));

    if (lid.isCustomConfigsLocation() && !index.data(kIsCustomConfigCorrect).toBool()) {
        QRect errorRect = customConfigErrorRect(option.rect);
        errorRect.adjust(-2*G_SCALE, -2*G_SCALE, 2*G_SCALE, 2*G_SCALE);
        if (errorRect.contains(point)) {
            return (int)ClickableRect::kErrorIcon;
        }
        return -1;
    }

    if (lid.isStaticIpsLocation() || lid.isCustomConfigsLocation()) {
        return -1;
    }
    if (index.data(kIsShowAsPremium).toBool()) {
        return -1;
    }

    QSharedPointer<IndependentPixmap> favIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/FAV_ICON_SELECTED");

    QRect rc(option.rect.left() + option.rect.width() - favIcon->width() - 59*G_SCALE,
             (option.rect.height() - favIcon->height())/2 + option.rect.top(),
             favIcon->width(), favIcon->height());
    rc.adjust(-2*G_SCALE, -2*G_SCALE, 2*G_SCALE, 2*G_SCALE);    //add a little more area for the convenience of clicking
    if (rc.contains(point)) {
        return (int)ClickableRect::kFavorite;
    }
    return (int)ClickableRect::kNone;
}

int CityItemDelegate::isInTooltipArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point, const IItemCacheData *cacheData) const
{
    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));

    // is broken custom config?
    if (lid.isCustomConfigsLocation() && !index.data(kIsCustomConfigCorrect).toBool()) {
        if (customConfigErrorRect(option.rect).contains(point)) {
            return (int)TooltipRect::kCustomConfigErrorMessage;
        }
    }

    int maxWidth = kCityItemMaxWidth;

    if (captionRect(option.rect, cacheData).contains(point)) {
        QString originalCaption = index.data().toString();
        QString truncatedCaption = CommonGraphics::maybeTruncatedText(
            originalCaption,
            FontManager::instance().getFont(16, QFont::Medium),
            (lid.isCustomConfigsLocation() ? maxWidth : kCityCaptionMaxWidth)*G_SCALE);
        if (originalCaption != truncatedCaption)
            return (int)TooltipRect::kItemCaption;
    }

    // Check if nickname is elided and mouse is over it
    if (!lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation()) {
        if (nicknameRect(option.rect, cacheData).contains(point)) {
            QString originalNickname = index.data(kDisplayNickname).toString();
            const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);
            QString truncatedNickname = CommonGraphics::maybeTruncatedText(originalNickname,
                                                                           FontManager::instance().getFont(16, QFont::Light),
                                                                           (maxWidth - 8)*G_SCALE - cache->pixmap(CityItemDelegateCache::kCityId).width());
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

        QRect rc = customConfigErrorRect(option.rect);
        QPoint pt = widget->mapToGlobal(QPoint(rc.center().x(), rc.top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = text;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.8;
        ti.parent = widget;
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
        ti.parent = widget;
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
        ti.parent = widget;
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
    else if (tooltipId == (int)TooltipRect::kItemCaption)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
    else if (tooltipId == (int)TooltipRect::kItemNickname)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    else
        WS_ASSERT(false);
}

QString CityItemDelegate::pingIconNameString(int connectionSpeedIndex) const
{
    if (connectionSpeedIndex == 0) {
        return "locations/LOCATION_PING_BARS0";
    } else if (connectionSpeedIndex == 1) {
        return "locations/LOCATION_PING_BARS1";
    } else if (connectionSpeedIndex == 2) {
        return "locations/LOCATION_PING_BARS2";
    } else {
        return "locations/LOCATION_PING_BARS3";
    }
    WS_ASSERT(false);
}

QRect CityItemDelegate::customConfigErrorRect(const QRect &itemRect) const
{
    // Custom config doesn't have 'a favorite' icon, so it's safe to assume that it's the last icon
    QSharedPointer<IndependentPixmap> errorIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/ERROR_ICON");
    int scaledX = itemRect.width() - errorIcon->width() - 27*G_SCALE;
    int scaledY = (itemRect.height() - errorIcon->height())/2;
    return QRect(itemRect.left() + scaledX, itemRect.top() + scaledY, errorIcon->width(), errorIcon->height());
}

QRect CityItemDelegate::captionRect(const QRect &itemRect, const IItemCacheData *cacheData) const
{
    const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);
    IndependentPixmap pixmapCaption = cache->pixmap(CityItemDelegateCache::kCityId);
    return QRect(itemRect.left() + 2*LOCATION_ITEM_MARGIN*G_SCALE + LOCATION_ITEM_FLAG_WIDTH*G_SCALE,
                 itemRect.top() + (itemRect.height() - pixmapCaption.height()) / 2,
                 pixmapCaption.width(), pixmapCaption.height());
}

QRect CityItemDelegate::nicknameRect(const QRect &itemRect, const IItemCacheData *cacheData) const
{
    const CityItemDelegateCache *cache = static_cast<const CityItemDelegateCache *>(cacheData);
    IndependentPixmap pixmapCaption = cache->pixmap(CityItemDelegateCache::kCityId);
    IndependentPixmap pixmapNick = cache->pixmap(CityItemDelegateCache::kNickId);

    QRect rcCaption(itemRect.left() + 2*LOCATION_ITEM_MARGIN*G_SCALE + LOCATION_ITEM_FLAG_WIDTH*G_SCALE,
                   itemRect.top() + (itemRect.height() - pixmapCaption.height()) / 2,
                   pixmapCaption.width(), pixmapCaption.height());

    return QRect(rcCaption.left() + rcCaption.width() + 8*G_SCALE,
                itemRect.top() + (itemRect.height() - pixmapNick.height()) / 2,
                pixmapNick.width(), pixmapNick.height());
}

} // namespace gui_locations
