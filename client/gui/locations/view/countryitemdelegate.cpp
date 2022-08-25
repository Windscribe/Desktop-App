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

namespace gui_locations {

// The text is drawn in QPixmap to speed up subsequent drawings
// The reason for this is that drawing text is quite slow and it speeds up significantly
class CountryItemDelegateCache : public IItemCacheData
{
public:
    explicit CountryItemDelegateCache(const QString &text, const QFont &font)
    {
        QFontMetrics fm(font);
        QRect rcText = fm.boundingRect(text);
        QPixmap pixmap(rcText.width()*2, rcText.height()*2);
        pixmap.setDevicePixelRatio(2);
        pixmap.fill(FontManager::instance().getMidnightColor());
        {
            QPainter painter(&pixmap);
            painter.setPen(Qt::white);
            painter.setFont(font);
            painter.drawText(QRect(0, 0, rcText.width() + 1, rcText.height() + 1), text);
        }
        pixmap_ = pixmap;
        //pixmap_.save("c:/1/aaa.png");
    }

    const QPixmap & getPixmap() const { return pixmap_; }

private:
    QPixmap pixmap_;
};


void CountryItemDelegate::paint(QPainter *painter, const ItemStyleOption &option, const QModelIndex &index, const IItemCacheData *cacheData) const
{
    painter->save();

    // background
    painter->fillRect(option.rect, FontManager::instance().getMidnightColor());

    int left_offs = option.rect.left();
    int top_offs = option.rect.top();

    // flag
    QSharedPointer<IndependentPixmap> flag = ImageResourcesSvg::instance().getFlag(index.data(COUNTRY_CODE).toString());
    if (flag)
    {
        const int pixmapFlagHeight = flag->height();
        flag->draw(left_offs + LOCATION_ITEM_MARGIN*G_SCALE, top_offs + (option.rect.height() - pixmapFlagHeight) / 2, painter);
    }

    // pro star
    if (index.data(IS_SHOW_AS_PREMIUM).toBool())
    {
        QSharedPointer<IndependentPixmap> proRegionStar = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_REGION_STAR_LIGHT");
        proRegionStar->draw(left_offs + 8 * G_SCALE, top_offs + (option.rect.height() - 16*G_SCALE) / 2 - 9*G_SCALE, painter);
    }

    double textOpacity = OPACITY_UNHOVER_TEXT + (OPACITY_FULL - OPACITY_UNHOVER_TEXT) * option.selectedOpacity();

    // text
    const CountryItemDelegateCache *countryItemDelegateCache = static_cast<const CountryItemDelegateCache *>(cacheData);
    //QFontMetrics fm(*FontManager::instance().getFont(16, true));
    //QRect rcText = fm.boundingRect(index.data().toString());
    //QPixmap pixmap(rcText.width(), rcText.height());
    //{
    //    QPainter p(&pixmap);
    //    p.drawText();
    //}
    //QPixmap pixmap()

    painter->setOpacity(textOpacity );
    //painter->setPen(Qt::white);
    //painter->setFont(*FontManager::instance().getFont(16, true));
    QRect rc = option.rect;
    rc.adjust(64*G_SCALE, 0, 0, 0);
    painter->drawPixmap(rc.left(), rc.top() + (rc.height() - countryItemDelegateCache->getPixmap().size().height()) / 2, countryItemDelegateCache->getPixmap());
    //painter->drawStaticText(rc.left(), rc.top() + (rc.height() - countryItemDelegateCache->get().size().height()) / 2 , countryItemDelegateCache->get());
    //painter->drawText(rc, Qt::AlignLeft | Qt::AlignVCenter, index.data().toString());*/

    // p2p icon
    if (index.data(IS_SHOW_P2P).toBool())
    {
        painter->setOpacity(OPACITY_HALF);

        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
        QRect p2pr = QRect(option.rect.width() - 65*G_SCALE,
                           (option.rect.height() - p->height()) / 2,
                           p->width(),
                           p->height());
        p->draw(left_offs + p2pr.x(), top_offs + p2pr.y(), painter);
    }

    LocationID lid = qvariant_cast<LocationID>(index.data(LOCATION_ID));
    if (lid.isBestLocation())
    {
        // 10gbps icon
        if (index.data(IS_10GBPS).toBool())
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
    int right = left_offs + static_cast<int>(option.rect.width() - 8*G_SCALE);
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
        int locationLoad = index.data(LOAD).toInt();
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
    return new CountryItemDelegateCache(index.data().toString(), *FontManager::instance().getFont(16, true));
}

bool CountryItemDelegate::isForbiddenCursor(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return false;
    }

    LocationID lid = qvariant_cast<LocationID>(index.data(LOCATION_ID));
    if (!lid.isBestLocation())
    {
        // if no child items
        return (!index.model()->index(0, 0, index).isValid());
    }
    return false;
}

int CountryItemDelegate::isInClickableArea(const QModelIndex &index, const QPoint &point, const QRect &itemRect) const
{
    return -1;
}

int CountryItemDelegate::isInTooltipArea(const QModelIndex &index, const QPoint &point) const
{
    return -1;
}

} // namespace gui_locations
