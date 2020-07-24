#include "locationitem.h"
#include <QPainter>
#include <QStyleOption>
#include <QtMath>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "widgetlocationssizes.h"
#include "../backend/types/locationid.h"
#include "dpiscalemanager.h"

namespace GuiLocations {


LocationItem::LocationItem(IWidgetLocationsInfo *widgetLocationsInfo, int id, const QString &countryCode, const QString &name, bool isShowP2P, PingTime timeMs, QVector<CityNode> &cities, bool forceExpand, bool isPremiumOnly) :
    timeMs_(timeMs, widgetLocationsInfo->isShowLatencyInMs()),
    citySubMenuState_(COLLAPSED), selectedInd_(-1), isCursorOverArrow_(false), isCursorOverP2P_(false), isPremiumOnly_(isPremiumOnly),
    isCursorOverConnectionMeter_(false),
    isCursorOverFavoriteIcon_(false),
    curWhiteLineValue_(0.0),
    widgetLocationsInfo_(widgetLocationsInfo)
{
    QFont *font = FontManager::instance().getFont(16, true);
    captionTextLayout_ = new QTextLayout(QObject::tr(name.toStdString().c_str()), *font);
    captionTextLayout_->beginLayout();
    captionTextLayout_->createLine();
    captionTextLayout_->endLayout();
    captionTextLayout_->setCacheEnabled(true);

    id_ = id;
    countryCode_ = countryCode;
    name_ = name;
    isShowP2P_ = isShowP2P;
    forceExpand_ = forceExpand;
    cityNodes_ = cities;
}

LocationItem::~LocationItem()
{
    delete captionTextLayout_;
}

void LocationItem::setSelected(int ind)
{
    selectedInd_ = ind;
    isCursorOverArrow_ = false;
    isCursorOverP2P_ = false;
}

int LocationItem::getSelected()
{
    return selectedInd_;
}

void LocationItem::setSelectedByCity(const LocationID &locationId)
{
    for (int i = 0; i < cityNodes_.count(); ++i)
    {
        if (cityNodes_[i].getLocationId() == locationId)
        {
            selectedInd_ = i + 1;
            return;
        }
    }
    selectedInd_ = 0;
}

int LocationItem::countVisibleItems()
{
    if (cityNodes_.count() > 0 && citySubMenuState_ != COLLAPSED && citySubMenuState_ != COLLAPSING)
    {
        return 1 + cityNodes_.count();
    }
    else
    {
        return 1;
    }
}

void LocationItem::drawLocationCaption(QPainter *painter, const QRect &rc)
{
    drawLocationCaption(painter, rc, selectedInd_ == 0);
}

void LocationItem::drawCities(QPainter *painter, const QRect &rc, QVector<int> &linesY, int &selectedY1, int &selectedY2)
{
    QRect clipRect(rc.left(), rc.top(), rc.width(), currentAnimationHeight());
    painter->setClipRect(clipRect);
    for (int i = 0; i < cityNodes_.count(); ++i)
    {
        QRect rcCity(rc.left(), rc.top() + i*WidgetLocationsSizes::instance().getItemHeight(), rc.width(), WidgetLocationsSizes::instance().getItemHeight());
        bool bSelected = ((i + 1) == selectedInd_);
        drawCityCaption(painter, cityNodes_[i], rcCity, bSelected);
        if (rcCity.bottom() >= clipRect.top() && rcCity.bottom() <= clipRect.bottom())
        {
            linesY << rcCity.bottom();
        }
        if (bSelected)
        {
            if (rcCity.top() <= clipRect.bottom())
            {
                selectedY1 = rcCity.top();
                if (rcCity.bottom() <= clipRect.bottom())
                {
                    selectedY2 = rcCity.bottom();
                }
                else
                {
                    selectedY2 = clipRect.bottom();
                }
            }
            else
            {
                selectedY1 = -1;
            }
        }
    }
    painter->setClipping(false);
}

// return true if something is changed and need redraw
bool LocationItem::mouseMoveEvent(QPoint &pt)
{
    //isCursorOverArrow_ = isCursorOverArrow(pt);

    isCursorOverP2P_ = isCursorOverP2P(pt);

    // detect mouse move for favorite icon
    bool bChanged = false;
    for (int i = 0; i < cityNodes_.count(); ++i)
    {
        QRect rcCity(0, (i+1)*WidgetLocationsSizes::instance().getItemHeight(), widgetLocationsInfo_->getWidth(), WidgetLocationsSizes::instance().getItemHeight());

        if (rcCity.contains(pt))
        {
            isCursorOverFavoriteIcon_ = isCursorOverFavoriteIcon(QPoint(pt.x(), pt.y() - (i+1)*WidgetLocationsSizes::instance().getItemHeight()), cityNodes_[i]);
            if (cityNodes_[i].isCursorOverFavoriteIcon() != isCursorOverFavoriteIcon_)
            {
                cityNodes_[i].setCursorOverFavoriteIcon(isCursorOverFavoriteIcon_);
                bChanged = true;
            }

            isCursorOverConnectionMeter_ = isCursorOverConnectionMeter(QPoint(pt.x(), pt.y() - (i+1)*WidgetLocationsSizes::instance().getItemHeight()));
            if (cityNodes_[i].isCursorOverConnectionMeter() != isCursorOverConnectionMeter_)
            {
                cityNodes_[i].setCursorOverConnectionMeter(isCursorOverConnectionMeter_);
                bChanged = true;
            }
        }

        if (rcCity.bottom() > currentAnimationHeight())
        {
            break;
        }

    }
    return bChanged;
}

void LocationItem::mouseLeaveEvent()
{
    isCursorOverArrow_ = false;
    //isCursorOverP2P_ = false;
    for (int i = 0; i < cityNodes_.count(); ++i)
    {
        cityNodes_[i].setCursorOverFavoriteIcon(false);
    }
}

QPoint LocationItem::getConnectionMeterCenter()
{
    return getConnectionMeterIconRect().center();
}

int LocationItem::detectOverFavoriteIconCity()
{
    for (int i = 0; i < cityNodes_.count(); ++i)
    {
        if (cityNodes_[i].isCursorOverFavoriteIcon())
        {
            return i;
        }
    }
    return -1;
}

void LocationItem::expand()
{
    Q_ASSERT(citySubMenuState_ == COLLAPSED || citySubMenuState_ == COLLAPSING);

    startAnimationHeight_ = currentAnimationHeight();
    int allCityHeight = cityNodes_.count() * WidgetLocationsSizes::instance().getItemHeight();
    endAnimationHeight_ = allCityHeight;
    //curAnimationDuration_ = (double)EXPAND_ANIMATION_DURATION * (double)(endAnimationHeight_ - startAnimationHeight_) / (double)allCityHeight * cityNodes_.count();
    curAnimationDuration_ = (double)EXPAND_ANIMATION_DURATION * (double)(endAnimationHeight_ - startAnimationHeight_) / (double)allCityHeight;

    citySubMenuState_ = EXPANDING;
    elapsedAnimationTimer_.start();

    startWhiteLineValue_ = curWhiteLineValue_;
    endWhiteLineValue_ = 1.0;
}

void LocationItem::setExpandedWithoutAnimation()
{
    if (cityNodes_.count() > 0)
    {
        citySubMenuState_ = EXPANDED;
    }
}

void LocationItem::collapse()
{
    Q_ASSERT(citySubMenuState_ == EXPANDED || citySubMenuState_ == EXPANDING);

    startAnimationHeight_ = currentAnimationHeight();
    int allCityHeight = cityNodes_.count() * WidgetLocationsSizes::instance().getItemHeight();
    endAnimationHeight_ = 0;
    //curAnimationDuration_ = (double)EXPAND_ANIMATION_DURATION * (double)(startAnimationHeight_ - endAnimationHeight_) / (double)allCityHeight * cityNodes_.count();
    curAnimationDuration_ = (double)EXPAND_ANIMATION_DURATION * (double)(startAnimationHeight_ - endAnimationHeight_) / (double)allCityHeight;

    citySubMenuState_ = COLLAPSING;
    elapsedAnimationTimer_.start();

    startWhiteLineValue_ = curWhiteLineValue_;
    endWhiteLineValue_ = 0.0;
}

void LocationItem::collapseForbidden()
{
    if ((citySubMenuState_ == EXPANDED || citySubMenuState_ == EXPANDING))
    {
        if (isForbidden(0))
        {
            collapse();
        }
    }
}

bool LocationItem::isExpandedOrExpanding()
{
    return citySubMenuState_ == EXPANDED || citySubMenuState_ == EXPANDING;
}

bool LocationItem::isForceExpand()
{
    return forceExpand_;
}

bool LocationItem::isAnimationFinished()
{
    return citySubMenuState_ != EXPANDING && citySubMenuState_ != COLLAPSING;
}

int LocationItem::currentAnimationHeight()
{
    double d = (double)elapsedAnimationTimer_.elapsed() / (double)curAnimationDuration_;

    curWhiteLineValue_ = startWhiteLineValue_ + (endWhiteLineValue_ - startWhiteLineValue_) * d;

    int curHeight = startAnimationHeight_ + (endAnimationHeight_ - startAnimationHeight_) * d;
    int allCityHeight = cityNodes_.count() * WidgetLocationsSizes::instance().getItemHeight();

    if (citySubMenuState_ == EXPANDING)
    {
        if (curHeight > allCityHeight)
        {
            citySubMenuState_ = EXPANDED;
            curWhiteLineValue_ = 1.0;
            return allCityHeight;
        }
        else
        {
            return curHeight;
        }
    }
    else if (citySubMenuState_ == COLLAPSING)
    {
        if (curHeight <= 0)
        {
            citySubMenuState_ = COLLAPSED;
            curWhiteLineValue_ = 0.0;
            return 0;
        }
        else
        {
            return curHeight;
        }
    }
    else if (citySubMenuState_ == EXPANDED)
    {
        curWhiteLineValue_ = 1.0;
        return allCityHeight;
    }
    else if (citySubMenuState_ == COLLAPSED)
    {
        curWhiteLineValue_ = 0.0;
        return 0;
    }
    else
    {
        Q_ASSERT(false);
        curWhiteLineValue_ = 0.0;
        return 0;
    }
}

bool LocationItem::isForbidden(int ind)
{
    Q_ASSERT(ind >= 0);
    if (ind == 0)
    {
        return false;
    }
    else
    {
        return cityNodes_[ind - 1].isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus();
    }
}

bool LocationItem::isForbidden(const CityNode &cityNode)
{
    return cityNode.isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus();
}

bool LocationItem::isNoConnection(int ind)
{
    Q_UNUSED(ind);

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

bool LocationItem::isSelectedDisabled()
{
    if (selectedInd_ > 0 )
    {
        return isForbidden(cityNodes_[selectedInd_-1]) ? false : cityNodes_[selectedInd_-1].isDisabled();
    }
    return false;
}

QString LocationItem::getCity(int ind)
{
    Q_ASSERT(ind < cityNodes_.count());
    return cityNodes_[ind].getLocationId().getCity();
}

int LocationItem::findCityInd(const LocationID &locationId)
{
    if (locationId.getCity().isEmpty())
    {
        return -1;
    }

    int ind = 0;
    Q_FOREACH(const CityNode &cn, cityNodes_)
    {
        if (cn.getLocationId() == locationId)
        {
            return ind;
        }
        ind++;
    }
    return -1;
}

QList<CityNode> LocationItem::cityNodes()
{
    QList<CityNode> result;

    for (int i = 0; i < cityNodes_.length(); i++)
    {
        result.append(cityNodes_[i]);
    }

    return result;
}

void LocationItem::changeSpeedConnection(const LocationID &locationId, PingTime timeMs)
{
    if (locationId.getCity().isEmpty())
    {
        timeMs_.setValue(timeMs);
    }
    else
    {
        for(int i = 0; i < cityNodes_.count(); ++i)
        {
            if (locationId == cityNodes_[i].getLocationId())
            {
                cityNodes_[i].setConnectionSpeed(timeMs);
                break;
            }
        }
    }
}

bool LocationItem::changeIsFavorite(const LocationID &locationId, bool isFavorite)
{
    for(int i = 0; i < cityNodes_.count(); ++i)
    {
        if (locationId == cityNodes_[i].getLocationId())
        {
            if (cityNodes_[i].isFavorite() != isFavorite)
            {
                cityNodes_[i].toggleIsFavorite();
                return true;
            }
            break;
        }
    }
    return false;
}

bool LocationItem::toggleFavorite(int cityInd)
{
    Q_ASSERT(cityInd >= 0 && cityInd < cityNodes_.count());
    cityNodes_[cityInd].toggleIsFavorite();
    return cityNodes_[cityInd].isFavorite();
}

void LocationItem::updateLangauge(QString name)
{
    captionTextLayout_->setText(name);

    // layout invalidated by setText -- remake
    captionTextLayout_->beginLayout();
    captionTextLayout_->createLine();
    captionTextLayout_->endLayout();
    captionTextLayout_->setCacheEnabled(true);
}

void LocationItem::updateScaling()
{
    QFont *font = FontManager::instance().getFont(16, true);
    captionTextLayout_->setFont(*font);
    captionTextLayout_->beginLayout();
    captionTextLayout_->createLine();
    captionTextLayout_->endLayout();
    captionTextLayout_->setCacheEnabled(true);

    for (int i = 0; i < cityNodes_.count(); ++i)
    {
        cityNodes_[i].updateScaling();
    }

    timeMs_.recreateTextLayout();
}

void LocationItem::drawLocationCaption(QPainter *painter, const QRect &rc, bool bSelected)
{
    painter->save();
    drawBottomLine(painter, rc.left() + 24*G_SCALE, rc.right(), rc.bottom(), curWhiteLineValue_);

    int leftOffs = 16*G_SCALE;

    // flag
    if (id_ != LocationID::STATIC_IPS_LOCATION_ID && id_ != LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
    {
        IndependentPixmap *flag = ImageResourcesSvg::instance().getFlag(countryCode_);
        int pixmapFlagHeight = flag->height() ;
        flag->draw(leftOffs, rc.top() + (rc.height() - pixmapFlagHeight) / 2, painter);
        leftOffs += flag->width() + 16*G_SCALE;
    }

    // pro star over flag
    if (widgetLocationsInfo_->isFreeSessionStatus() && isPremiumOnly_)
    {
        IndependentPixmap *proRegionStar = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_REGION_STAR_LIGHT");
        proRegionStar->draw(8 * G_SCALE, rc.top() + (rc.height() - 16*G_SCALE) / 2 - 9*G_SCALE, painter);
    }

    QRectF boundingRect = captionTextLayout_->boundingRect();

    // P2P
    if (isShowP2P_)
    {
        painter->setOpacity(0.5);
        IndependentPixmap *nop2pPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
        nop2pPixmap->draw(rc.right() - 65 * G_SCALE, rc.top() + (rc.height() - nop2pPixmap->height()) / 2, painter);
    }

    // text
    painter->setPen(QColor(0xFF, 0xFF , 0xFF));
    painter->setOpacity(bSelected ? 1.0 : 0.5);
    captionTextLayout_->draw(painter, QPoint(leftOffs, rc.top() + (rc.height() - boundingRect.height()) / 2 - 1));

    bool allNodesDisabled = true;
    foreach (CityNode cityNode, cityNodes_)
    {
        // API returns disabled for all premium locations when from free user
        bool disabled = isForbidden(cityNode) ? false : cityNode.isDisabled();

        if (!disabled)
        {
            allNodesDisabled = false;
            break;
        }
    }

    if (allNodesDisabled) // all cities disabled or no cities
    {
        IndependentPixmap *constructionPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/UNDER_CONSTRUCTION_ICON");
        int constructionPixmapWidth = constructionPixmap->width();
        int constructionPixmapHeight = constructionPixmap->height();
        leftOffs = rc.width() - constructionPixmapWidth - 16 * G_SCALE;
        painter->translate(QPoint(leftOffs + constructionPixmapWidth / 2, rc.top() + rc.height() / 2));
        painter->setOpacity(bSelected ? 1.0 : 0.3);
        constructionPixmap->draw(-constructionPixmapWidth / 2, -constructionPixmapHeight / 2, painter);
    }
    else     // plus/close
    {
        IndependentPixmap *expandPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");
        int expandPixmapWidth = expandPixmap->width();
        int expandPixmapHeight = expandPixmap->height();
        leftOffs = rc.width() - expandPixmapWidth - 16 * G_SCALE;
        painter->translate(QPoint(leftOffs + expandPixmapWidth / 2, rc.top() + rc.height() / 2));
        painter->rotate(45 * curWhiteLineValue_);
        painter->setOpacity(bSelected ? 1.0 : 0.3);
        expandPixmap->draw(-expandPixmapWidth / 2, -expandPixmapHeight / 2, painter);
    }

    painter->restore();
}

void LocationItem::drawCityCaption(QPainter *painter, CityNode &cityNode, const QRect &rc, bool bSelected)
{
    painter->save();
    painter->setOpacity(1.0);
    drawBottomLine(painter, rc.left() + 24*G_SCALE, rc.right(), rc.bottom(), 0.0);

    if (cityNode.isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus())
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


    painter->setPen(QColor(0xFF, 0xFF , 0xFF));
    painter->setOpacity(bSelected && !isSelectedDisabled() ? 1.0 : 0.5);

    QRectF boundingRect1 = cityNode.getCaption1TextLayout()->boundingRect();
    cityNode.getCaption1TextLayout()->draw(painter, QPoint(64 * G_SCALE, rc.top() + (rc.height() - boundingRect1.height()) / 2 - 1*G_SCALE));

    QRectF boundingRect2 = cityNode.getCaption2TextLayout()->boundingRect();
    cityNode.getCaption2TextLayout()->draw(painter, QPoint(68 * G_SCALE + boundingRect1.width(), rc.top() + (rc.height() - boundingRect2.height()) / 2 - 1*G_SCALE));

    painter->setOpacity(1.0);

    // API returns disabled for all premium locations when from free user
    bool disabled = isForbidden(cityNode) ? false : cityNode.isDisabled();

    if (disabled)
    {
        painter->setOpacity(.3);
        IndependentPixmap *disabledPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/UNDER_CONSTRUCTION_ICON");
        disabledPixmap->draw(rc.right() - disabledPixmap->width() - 16 * G_SCALE, rc.top() + (rc.height() - disabledPixmap->height()) / 2, painter);
    }
    else
    {
        if (widgetLocationsInfo_->isShowLatencyInMs())
        {
            int latencyRectWidth = 33 * G_SCALE;
            int latencyRectHeight = 20 * G_SCALE;
            QRect latencyRect(rc.right() - latencyRectWidth - 8 * G_SCALE, rc.top() + ((rc.height() - latencyRectHeight) / 2 - 1*G_SCALE), latencyRectWidth, latencyRectHeight);

            // painter->setFont(*FontManager::instance().getFont(12* g_pixelRatio,false));
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(QBrush(QColor(40, 45, 61)));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(latencyRect, 4*G_SCALE, 4*G_SCALE);

            QRectF boundingRect3 = cityNode.getLatencyMsTextLayout()->boundingRect();
            boundingRect3.moveCenter(latencyRect.center());

            painter->setPen(QColor(255, 255, 255));

            cityNode.getLatencyMsTextLayout()->draw(painter, QPoint(boundingRect3.left() + 2*G_SCALE, boundingRect3.top() + 1*G_SCALE));

            //cityNode.getLatencyMsTextLayout()->draw(painter, QPoint(rc.right() - boundingRect3.width() - 16, rc.top() + (rc.height() - boundingRect3.height()) / 2 - 1));
        }
        else
        {
            IndependentPixmap *pingBarPixmap;
            if (cityNode.timeMs().toConnectionSpeed() == 0 || (cityNode.isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus()))
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

bool LocationItem::isCursorOverP2P(const QPoint &pt)
{
    Q_UNUSED(pt);

    if (isShowP2P_)
    {
        IndependentPixmap *noP2pPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
        int noP2pPixmapHeight = noP2pPixmap->height();

        QRect rc((333 - 65) * G_SCALE, (48 * G_SCALE - noP2pPixmapHeight) / 2, noP2pPixmap->width(), noP2pPixmapHeight);
        return rc.contains(pt);
    }
    else
    {
        return false;
    }
}

bool LocationItem::isCursorOverConnectionMeter(const QPoint &pt)
{
    return getConnectionMeterIconRect().contains(pt);
}

bool LocationItem::isCursorOverFavoriteIcon(const QPoint &pt, const CityNode &cityNode)
{
    if (cityNode.isShowPremiumStar() && widgetLocationsInfo_->isFreeSessionStatus())
    {
        return false;
    }

    IndependentPixmap *favoritePixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/DESELECTED_FAV_ICON");
    int favoritePixmapWidth = favoritePixmap->width();
    int favoritePixmapHeight = favoritePixmap->height();

    QRect rc(16 * G_SCALE, (48 * G_SCALE - favoritePixmapHeight) / 2, favoritePixmapWidth, favoritePixmapHeight);
    return rc.contains(pt);
}

QRect LocationItem::getConnectionMeterIconRect()
{
    IndependentPixmap *pingMeterPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/LOCATION_PING_BARS0");

    QRect rc(333*G_SCALE - pingMeterPixmap->width() - 16 * G_SCALE,
             17*G_SCALE,
             pingMeterPixmap->width(),
             pingMeterPixmap->height());

    return rc;
}

void LocationItem::drawBottomLine(QPainter *painter, int left, int right, int bottom, double whiteValue)
{
    // draw white line with whiteValue from 0.0 to 1.0
    if( qFabs(1.0 - whiteValue) < 0.000001 )
    {
        QPen pen(Qt::white);
        pen.setWidth(1);
        painter->setPen(pen);
        painter->drawLine(left, bottom, right, bottom);
        painter->drawLine(left, bottom - 1, right, bottom - 1); //
    }
    else
    {
        QPen pen(QColor(0x29, 0x2E, 0x3E));
        pen.setWidth(1);
        painter->setPen(pen);
        painter->drawLine(left, bottom, right, bottom);
        painter->drawLine(left, bottom - 1, right, bottom - 1); //

        if(whiteValue > 0.000001 )
        {
            int w = (right - left) * whiteValue;
            QPen pen(Qt::white);
            pen.setWidth(1);
            painter->setPen(pen);
            painter->drawLine(left, bottom, left + w, bottom);
            painter->drawLine(left, bottom - 1, left + w, bottom - 1); //
        }
    }
}

} // namespace GuiLocations
