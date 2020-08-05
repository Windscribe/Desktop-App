#include "staticipdeviceitem.h"

#include "widgetlocationssizes.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace GuiLocations {

StaticIpDeviceItem::StaticIpDeviceItem(IWidgetLocationsInfo *widgetLocationsInfo) :
    isSelected_(false)
  , isCursorOverCaption1Text_(false)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , deviceName_("")
{

}

void StaticIpDeviceItem::setSelected(bool isSelected)
{
    isSelected_ = isSelected;
}

bool StaticIpDeviceItem::isSelected()
{
    return isSelected_;
}

LocationID StaticIpDeviceItem::getLocationId()
{
    return LocationID(LocationID::RIBBON_ITEM_STATIC_IP);
}

QString StaticIpDeviceItem::getCountryCode() const
{
    return "";
}

int StaticIpDeviceItem::getPingTimeMs()
{
    return 0;
}

QString StaticIpDeviceItem::getCaption1TruncatedText()
{
    return QObject::tr("Add Static IP");
}

QString StaticIpDeviceItem::getCaption1FullText()
{
    return QObject::tr("Add Static IP");
}

QPoint StaticIpDeviceItem::getCaption1TextCenter()
{
    return QPoint(0,0);
}

int StaticIpDeviceItem::countVisibleItems()
{
    return 1;
}

void StaticIpDeviceItem::drawLocationCaption(QPainter *painter, const QRect &rc)
{
    painter->save();

    painter->setOpacity(1.0);
    painter->translate(rc.left(), rc.top()-3* G_SCALE);
    painter->fillRect(QRect(0,0, WINDOW_WIDTH * G_SCALE, 48 * G_SCALE), FontManager::instance().getCarbonBlackColor());

    // Device Name text
    painter->setPen(QColor(0xFF, 0xFF , 0xFF));
    painter->setOpacity(0.5);
    int textPosY = 28 *G_SCALE;
    painter->setFont(*FontManager::instance().getFont(16, true));
    painter->drawText(QPoint(16*G_SCALE, textPosY), deviceName_);

    // Add static ip text
    painter->setOpacity(isSelected_ ? 1.0  : OPACITY_UNHOVER_TEXT);
    QFont addStaticIpTextFont = *FontManager::instance().getFont(16, false);
    painter->setFont(addStaticIpTextFont);
    QString addStaticIpText = QObject::tr("Add Static IP");
    painter->drawText(QPoint(WINDOW_WIDTH * G_SCALE - CommonGraphics::textWidth(addStaticIpText, addStaticIpTextFont) - 42 * G_SCALE,
                             textPosY), addStaticIpText);

    // external link pixmap
    painter->setOpacity(isSelected_ ? 1.0  : OPACITY_UNHOVER_ICON_TEXT);
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/EXTERNAL_LINK_ICON");
    p->draw((WINDOW_WIDTH - 32) * G_SCALE, 15*G_SCALE, painter);

    painter->setOpacity(1.0);
    painter->restore();
}

bool StaticIpDeviceItem::mouseMoveEvent(QPoint &pt)
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

void StaticIpDeviceItem::mouseLeaveEvent()
{
    // do nothing
}

bool StaticIpDeviceItem::isCursorOverConnectionMeter()
{
    return false;
}

bool StaticIpDeviceItem::isCursorOverCaption1Text()
{
    return isCursorOverCaption1Text_;
}

bool StaticIpDeviceItem::isCursorOverFavouriteIcon()
{
    return false;
}

bool StaticIpDeviceItem::detectOverFavoriteIconCity()
{
    return false;
}

bool StaticIpDeviceItem::isForbidden()
{
    return false;
}

bool StaticIpDeviceItem::isNoConnection()
{
    return false;
}

bool StaticIpDeviceItem::isDisabled()
{
    return false;
}

int StaticIpDeviceItem::findCityInd(const LocationID & /*locationId*/)
{
    return 0;
}

void StaticIpDeviceItem::changeSpeedConnection(const LocationID & /*locationId*/, PingTime /*timeMs*/)
{
    // do nothing
}

bool StaticIpDeviceItem::changeIsFavorite(const LocationID & /*locationId*/, bool /*isFavorite*/)
{
    return false;
}

bool StaticIpDeviceItem::toggleFavorite()
{
    return false;
}

void StaticIpDeviceItem::updateScaling()
{
    // do nothing
}

void StaticIpDeviceItem::setDeviceName(QString deviceName)
{
    deviceName_ = deviceName;
}

}
