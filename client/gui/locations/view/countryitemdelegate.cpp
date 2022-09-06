#include "countryitemdelegate.h"

#include <QPainter>
#include <QStaticText>
#include <QtMath>

#include "../locationsmodel_roles.h"
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
    painter->fillRect(option.rect, FontManager::instance().getMidnightColor());

    int left_offs = option.rect.left();
    int top_offs = option.rect.top();

    // flag
    QSharedPointer<IndependentPixmap> flag = ImageResourcesSvg::instance().getFlag(index.data(kCountryCode).toString());
    if (flag)
    {
        const int pixmapFlagHeight = flag->height();
        flag->draw(left_offs + LOCATION_ITEM_MARGIN*2*G_SCALE, top_offs + (option.rect.height() - pixmapFlagHeight) / 2, painter);
    }

    // pro star
    if (index.data(kIsShowAsPremium).toBool())
    {
        QSharedPointer<IndependentPixmap> proRegionStar = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_REGION_STAR_LIGHT");
        proRegionStar->draw(left_offs + 8 * G_SCALE, top_offs + (option.rect.height() - 16*G_SCALE) / 2 - 9*G_SCALE, painter);
    }


    // text
    double textOpacity = OPACITY_UNHOVER_TEXT + (OPACITY_FULL - OPACITY_UNHOVER_TEXT) * option.selectedOpacity();
    const CountryItemDelegateCache *cache = static_cast<const CountryItemDelegateCache *>(cacheData);
    painter->setOpacity(textOpacity );
    QRect rc = option.rect;
    rc.adjust(64*G_SCALE, 0, 0, 0);
    IndependentPixmap pixmap = cache->pixmap(CountryItemDelegateCache::kCaptionId);
    pixmap.draw(rc.left(), rc.top() + (rc.height() - pixmap.height()) / 2, painter);

    // p2p icon
    if (index.data(kIsShowP2P).toBool())
    {
        painter->setOpacity(OPACITY_HALF);

        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
        QRect p2pr = QRect(option.rect.width() - 57*G_SCALE,
                           (option.rect.height() - p->height()) / 2,
                           p->width(),
                           p->height());
        p->draw(left_offs + p2pr.x(), top_offs + p2pr.y(), painter);
    }

    LocationID lid = qvariant_cast<LocationID>(index.data(kLocationId));
    if (lid.isBestLocation())
    {
        // 10gbps icon
        if (index.data(kIs10Gbps).toBool())
        {
            painter->setOpacity(OPACITY_FULL);
            QSharedPointer<IndependentPixmap> tenGbpsPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/10_GBPS_ICON");
            tenGbpsPixmap->draw(left_offs + option.rect.width() - LOCATION_ITEM_MARGIN*G_SCALE - tenGbpsPixmap->width(), top_offs + (option.rect.height() - tenGbpsPixmap->height()) / 2, painter);
        }
    }
    // plus/cross
    else
    {
        double plusIconOpacity_ = OPACITY_THIRD + (OPACITY_FULL - OPACITY_THIRD) * option.selectedOpacity();
        painter->setOpacity(plusIconOpacity_);
        QSharedPointer<IndependentPixmap> expandPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");

        // this part is kind of magical - could use some more clear math
        painter->save();
        painter->translate(QPoint(left_offs + option.rect.width() - LOCATION_ITEM_MARGIN*G_SCALE - expandPixmap->width()/2, top_offs + option.rect.height()/2));
        painter->rotate(45 * option.expandedProgress());
        expandPixmap->draw( - expandPixmap->width() / 2,
                           - expandPixmap->height()/ 2,
                            painter);
        painter->restore();
    }

    // bottom lines
    int left = left_offs + static_cast<int>(24 * G_SCALE);
    int right = left_offs + static_cast<int>(option.rect.width());
    int bottom = top_offs + option.rect.height() - 1; // 1 is not scaled since we want bottom-most pixel inside geometry
    painter->setOpacity(1.0);

    // TODO: lines not scaled since we draw just single pixels
    // background line (darker line)
    QPen pen(QColor(0x29, 0x2E, 0x3E));
    pen.setWidth(1);
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

    // top-most line (white)
    if( qFabs(1.0 - option.expandedProgress()) < 0.000001 )
    {
        QPen white_pen(Qt::white);
        white_pen.setWidth(1);
        painter->setPen(white_pen);
        painter->drawLine(left, bottom, right, bottom);
        painter->drawLine(left, bottom - 1, right, bottom - 1);
    }
    else if (option.expandedProgress() > 0.000001)
    {
        int w = (right - left) * option.expandedProgress();
        QPen white_pen(Qt::white);
        white_pen.setWidth(1);
        painter->setPen(white_pen);
        painter->drawLine(left, bottom, left + w, bottom);
        painter->drawLine(left, bottom - 1, left + w, bottom - 1);
    }

    painter->restore();
}

IItemCacheData *CountryItemDelegate::createCacheData(const QModelIndex &index) const
{
    CountryItemDelegateCache *cache = new CountryItemDelegateCache();
    cache->add(CountryItemDelegateCache::kCaptionId, index.data().toString(), *FontManager::instance().getFont(16, true), DpiScaleManager::instance().curDevicePixelRatio());
    return cache;
}

void CountryItemDelegate::updateCacheData(const QModelIndex &index, IItemCacheData *cacheData) const
{
    CountryItemDelegateCache *cache = static_cast<CountryItemDelegateCache *>(cacheData);
    cache->updateIfTextChanged(CountryItemDelegateCache::kCaptionId, index.data().toString(), *FontManager::instance().getFont(16, true), DpiScaleManager::instance().curDevicePixelRatio());
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

    return (int)TooltipRect::kNone;
}

void CountryItemDelegate::tooltipEnterEvent(const ItemStyleOption &option, const QModelIndex &index, int tooltipId, const IItemCacheData *cacheData) const
{
    Q_ASSERT(dynamic_cast<QWidget *>(option.styleObject) != nullptr);
    if (tooltipId == (int)TooltipRect::kP2P) {
        QWidget *widget = static_cast<QWidget *>(option.styleObject);
        QRect rc = p2pRect(option.rect);
        QPoint pt = widget->mapToGlobal(QPoint(rc.center().x(), rc.top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_P2P);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = widget->tr("File Sharing Frowned Upon");
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    } else {
        Q_ASSERT(false);
    }
}

void CountryItemDelegate::tooltipLeaveEvent(int tooltipId) const
{
    if (tooltipId == (int)TooltipRect::kP2P)
        TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
    else
        Q_ASSERT(false);
}

QRect CountryItemDelegate::p2pRect(const QRect &itemRect) const
{
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
    return QRect(itemRect.width() - 57*G_SCALE + itemRect.left(),
                (itemRect.height() - p->height()) / 2 + itemRect.top(),
                 p->width(),
                 p->height());
}

} // namespace gui_locations
