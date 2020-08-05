#include "configfooteritem.h"
#include "widgetlocationssizes.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace GuiLocations {

ConfigFooterItem::ConfigFooterItem(IWidgetLocationsInfo *widgetLocationsInfo) :
    isSelected_(false)
  , isCursorOverCaption1Text_(false)
  , widgetLocationsInfo_(widgetLocationsInfo)
{

}

void ConfigFooterItem::setSelected(bool isSelected)
{
    isSelected_ = isSelected;
}

bool ConfigFooterItem::isSelected()
{
    return isSelected_;
}

LocationID ConfigFooterItem::getLocationId()
{
    return LocationID(LocationID::RIBBON_ITEM_CONFIG);
}

QString ConfigFooterItem::getCountryCode() const
{
    return "";
}

int ConfigFooterItem::getPingTimeMs()
{
    return 0;
}

QString ConfigFooterItem::getCaption1TruncatedText()
{
    return QObject::tr("Add Config Location");
}

QString ConfigFooterItem::getCaption1FullText()
{
    return QObject::tr("Add Config Location");
}

QPoint ConfigFooterItem::getCaption1TextCenter()
{
    return QPoint(0,0);
}

int ConfigFooterItem::countVisibleItems()
{
    return 1;
}

void ConfigFooterItem::drawLocationCaption(QPainter *painter, const QRect &rc)
{
    painter->save();

    painter->setOpacity(1.0);
    painter->translate(rc.left(), rc.top()-3);
    painter->fillRect(QRect(0,0, WINDOW_WIDTH*G_SCALE, 48*G_SCALE), FontManager::instance().getCarbonBlackColor());

    painter->setPen(QColor(0xFF, 0xFF , 0xFF));
    painter->setOpacity(isSelected_ ? 1.0  : OPACITY_UNHOVER_TEXT);
    int textPosY = 28*G_SCALE;
    painter->setFont(*FontManager::instance().getFont(16, true));
    painter->drawText(QPoint(16*G_SCALE, textPosY), QObject::tr("Add Config Location"));

    painter->setOpacity(isSelected_ ? 1.0  : OPACITY_UNHOVER_ICON_TEXT);

    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("locations/FOLDER_ICON");
    p->draw((WINDOW_WIDTH - 32) *G_SCALE, 15*G_SCALE, painter);

    painter->setOpacity(1.0);
    painter->restore();
}

bool ConfigFooterItem::mouseMoveEvent(QPoint &pt)
{
    isCursorOverCaption1Text_ = false;

    bool bChanged = false;
    QRect rcCity(0, 0, widgetLocationsInfo_->getWidth(), WidgetLocationsSizes::instance().getItemHeight());
    if (rcCity.contains(pt))
    {
        bChanged = true;
    }

    return bChanged;
}

void ConfigFooterItem::mouseLeaveEvent()
{
    // do nothing
}

bool ConfigFooterItem::isCursorOverConnectionMeter()
{
    return false;
}

bool ConfigFooterItem::isCursorOverCaption1Text()
{
    return isCursorOverCaption1Text_;
}

bool ConfigFooterItem::isCursorOverFavouriteIcon()
{
    return false;
}

bool ConfigFooterItem::detectOverFavoriteIconCity()
{
    return false;
}

bool ConfigFooterItem::isForbidden()
{
    return false;
}

bool ConfigFooterItem::isNoConnection()
{
    return false;
}

bool ConfigFooterItem::isDisabled()
{
    return false;
}

int ConfigFooterItem::findCityInd(const LocationID & /*locationId*/)
{
    return 0;
}

void ConfigFooterItem::changeSpeedConnection(const LocationID & /*locationId*/, PingTime /*timeMs*/)
{
    // do nothing
}

bool ConfigFooterItem::changeIsFavorite(const LocationID & /*locationId*/, bool /*isFavorite*/)
{
    return false;
}

bool ConfigFooterItem::toggleFavorite()
{
    return false;
}

void ConfigFooterItem::updateScaling()
{
    // do nothing
}

}
