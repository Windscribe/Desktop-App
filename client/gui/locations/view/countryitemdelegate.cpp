#include "countryitemdelegate.h"

#include <QPainter>
#include <QStaticText>
#include <QtMath>

#include "locations/locationsmodel_roles.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "types/locationid.h"
#include "textpixmap.h"
#include "clickableandtooltiprects.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"

namespace  {

class CountryItemDelegateCache : public gui_locations::IItemCacheData, public gui_locations::TextPixmaps
{
public:
    static constexpr int kCaptionId = 0;
};

}

namespace gui_locations {

void CountryItemDelegate::paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const
{
    painter->save();

    // background
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(OPACITY_FULL);
    painter->fillRect(option.rect, FontManager::instance().getMidnightColor());

    int left_offs = option.rect.left();
    int top_offs = option.rect.top();

    // flag
    QSharedPointer<IndependentPixmap> flag = ImageResourcesSvg::instance().getCircleFlag(index.data(kCountryCode).toString());
    QRect flagRect = QRect(left_offs + 16*G_SCALE, top_offs + (option.rect.height() - flag->height()) / 2, flag->width(), flag->height());
    if (flag) {
        flag->draw(flagRect, painter);
    }

    double textOpacity = OPACITY_UNHOVER_TEXT + (OPACITY_FULL - OPACITY_UNHOVER_TEXT) * option.selectedOpacity();

    painter->setOpacity(OPACITY_FULL);
    QPen penLoad(QColor(53, 61, 73));
    penLoad.setWidth(2*G_SCALE);
    // Draw a full circle with grey color: this is (255, 255, 255, 51) blended with (2, 13, 28).
    // Drawing (255, 255, 255, 51) is not visible since it draws on top of some transparent pixels.
    penLoad.setColor(QColor(53, 61, 73));
    painter->setPen(penLoad);
    painter->drawArc(flagRect, 0, 360 * 16);

    if (option.isShowLocationLoad()) {
        int locationLoad = index.data(kLoad).toInt();
        if (locationLoad > 0) {
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
            painter->drawArc(flagRect, 180 * 16, -360 * locationLoad * 16 / 100);
        }
    }

    // pro star
    if (index.data(kIsShowAsPremium).toBool()) {
        QSharedPointer<IndependentPixmap> proRegionStar = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_REGION_STAR_LIGHT");
        proRegionStar->draw(left_offs + 8 * G_SCALE, top_offs + (option.rect.height() - 16*G_SCALE) / 2 - 9*G_SCALE, painter);
    }

    // text
    const CountryItemDelegateCache *cache = static_cast<const CountryItemDelegateCache *>(cacheData);
    painter->setOpacity(textOpacity );
    QRect rc = option.rect;
    rc.adjust(56*G_SCALE, 0, 0, 0);
    IndependentPixmap pixmap = cache->pixmap(CountryItemDelegateCache::kCaptionId);
    pixmap.draw(rc.left(), rc.top() + (rc.height() - pixmap.height()) / 2, painter);

    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (lid.isBestLocation()) {
        if (index.data(kIs10Gbps).toBool()) {
            painter->setOpacity(OPACITY_FULL);
            int xOffset = 0;
            if (index.data(kIsShowP2P).toBool()) {
                xOffset = -24*G_SCALE;
            }
            QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/10_GBPS_ICON");
            QRect tenGbpsRect = QRect(option.rect.width() - 40*G_SCALE + xOffset - p->width(), (option.rect.height() - p->height()) / 2, p->width(), p->height());
            p->draw(left_offs + tenGbpsRect.x(), top_offs + tenGbpsRect.y(), painter);
        }
    }

    // p2p icon
    if (index.data(kIsShowP2P).toBool()) {
        painter->setOpacity(OPACITY_FULL);
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
        QRect p2pr = QRect(option.rect.width() - 40*G_SCALE - p->width(),
                           (option.rect.height() - p->height()) / 2,
                           p->width(),
                           p->height());
        p->draw(left_offs + p2pr.x(), top_offs + p2pr.y(), painter);
    }

    if (lid.isBestLocation()) {
        double arrowOpacity = OPACITY_THIRD + (OPACITY_FULL - OPACITY_THIRD) * option.selectedOpacity();
        painter->setOpacity(arrowOpacity);
        QSharedPointer<IndependentPixmap> arrowPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/ARROW_RIGHT");
        arrowPixmap->draw(left_offs + option.rect.width() - 16*G_SCALE - arrowPixmap->width(), top_offs + (option.rect.height() - arrowPixmap->height()) / 2, painter);
    } else { // plus/cross
        double plusIconOpacity_ = OPACITY_THIRD + (OPACITY_FULL - OPACITY_THIRD) * option.selectedOpacity();
        painter->setOpacity(plusIconOpacity_);
        QSharedPointer<IndependentPixmap> togglePixmap;
        if (qFabs(1.0 - option.expandedProgress()) < 0.000001) {
            togglePixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/COLLAPSE_ICON");
        } else {
            togglePixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");
        }
        togglePixmap->draw(left_offs + option.rect.width() - 16*G_SCALE - togglePixmap->width(),
                           top_offs + (option.rect.height() - togglePixmap->height()) / 2,
                           painter);
    }

    painter->restore();
}

IItemCacheData *CountryItemDelegate::createCacheData(const QModelIndex &index) const
{
    CountryItemDelegateCache *cache = new CountryItemDelegateCache();
    bool isShowP2P = index.data(kIsShowP2P).toBool();
    int maxWidth = kCountryItemMaxWidth*G_SCALE;
    if (isShowP2P) {
        maxWidth -= 32*G_SCALE;
    }
    bool isShow10Gbps = index.data(kIs10Gbps).toBool();
    if (isShow10Gbps) {
        maxWidth -= 32*G_SCALE;
    }

    QString countryCaption = CommonGraphics::maybeTruncatedText(index.data().toString(),
                                                                FontManager::instance().getFont(16, QFont::Medium),
                                                                static_cast<int>(maxWidth));
    cache->add(CountryItemDelegateCache::kCaptionId, countryCaption, FontManager::instance().getFont(16, QFont::Medium), DpiScaleManager::instance().curDevicePixelRatio());
    return cache;
}

void CountryItemDelegate::updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const
{
    CountryItemDelegateCache *cache = static_cast<CountryItemDelegateCache *>(cacheData);
    bool isShowP2P = index.data(kIsShowP2P).toBool();
    int maxWidth = kCountryItemMaxWidth*G_SCALE;
    if (isShowP2P) {
        maxWidth -= 32*G_SCALE;
    }
    bool isShow10Gbps = index.data(kIs10Gbps).toBool();
    if (isShow10Gbps) {
        maxWidth -= 32*G_SCALE;
    }
    QString countryCaption = CommonGraphics::maybeTruncatedText(index.data().toString(),
                                                                FontManager::instance().getFont(16, QFont::Medium),
                                                                static_cast<int>(maxWidth));
    cache->updateIfTextChanged(CountryItemDelegateCache::kCaptionId, countryCaption, FontManager::instance().getFont(16, QFont::Medium), DpiScaleManager::instance().curDevicePixelRatio());
}

bool CountryItemDelegate::isForbiddenCursor(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;

    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (!lid.isBestLocation())
        return (!index.model()->index(0, 0, index).isValid());  // if no child items

    return false;
}

int CountryItemDelegate::isInClickableArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point) const
{
    return (int)ClickableRect::kNone;
}

int CountryItemDelegate::isInTooltipArea(const ItemStyleOption &option, const QModelIndex &index, const QPoint &point, const IItemCacheData *cacheData) const
{
    // p2p icon
    if (index.data(kIsShowP2P).toBool()) {
        if (p2pRect(option.rect).contains(point)) {
            return (int)TooltipRect::kP2P;
        }
    }

    // Check if country caption is elided and mouse is over it
    if (captionRect(option.rect, cacheData).contains(point)) {
        QString originalCaption = index.data().toString();
        bool isShowP2P = index.data(kIsShowP2P).toBool();
        int maxWidth = isShowP2P ? (kCountryItemMaxWidth - 32)*G_SCALE : kCountryItemMaxWidth*G_SCALE;

        QString truncatedCaption = CommonGraphics::maybeTruncatedText(originalCaption,
                                                                     FontManager::instance().getFont(16, QFont::Medium),
                                                                     static_cast<int>(maxWidth));
        if (originalCaption != truncatedCaption)
            return (int)TooltipRect::kCountryCaption;
    }

    return (int)TooltipRect::kNone;
}

void CountryItemDelegate::tooltipEnterEvent(const ItemStyleOption &option, const QModelIndex &index, int tooltipId, const IItemCacheData *cacheData) const
{
    WS_ASSERT(dynamic_cast<QWidget *>(option.styleObject) != nullptr);
    QWidget *widget = static_cast<QWidget *>(option.styleObject);

    if (tooltipId == (int)TooltipRect::kP2P) {
        QRect rc = p2pRect(option.rect);
        QPoint pt = widget->mapToGlobal(QPoint(rc.center().x(), rc.top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_P2P);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = QWidget::tr("File Sharing Frowned Upon");
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    } else if (tooltipId == (int)TooltipRect::kCountryCaption) {
        QRect rc = captionRect(option.rect, cacheData);
        QPoint pt = widget->mapToGlobal(QPoint(rc.x() + rc.width()/2, rc.top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = index.data().toString();
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    } else {
        WS_ASSERT(false);
    }
}

void CountryItemDelegate::tooltipLeaveEvent(int tooltipId) const
{
    if (tooltipId == (int)TooltipRect::kP2P)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
    else if (tooltipId == (int)TooltipRect::kCountryCaption)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    else
        WS_ASSERT(false);
}

QRect CountryItemDelegate::p2pRect(const QRect &itemRect) const
{
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
    return QRect(itemRect.width() - 40*G_SCALE - p->width(),
                (itemRect.height() - p->height()) / 2 + itemRect.top(),
                 p->width(),
                 p->height());
}

QRect CountryItemDelegate::captionRect(const QRect &itemRect, const IItemCacheData *cacheData) const
{
    const CountryItemDelegateCache *cache = static_cast<const CountryItemDelegateCache *>(cacheData);
    IndependentPixmap pixmap = cache->pixmap(CountryItemDelegateCache::kCaptionId);
    return QRect(itemRect.left() + 56*G_SCALE,
                itemRect.top() + (itemRect.height() - pixmap.height()) / 2,
                pixmap.width(),
                pixmap.height());
}

} // namespace gui_locations
