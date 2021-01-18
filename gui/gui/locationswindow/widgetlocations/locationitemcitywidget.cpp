#include "locationitemcitywidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "widgetlocationssizes.h"
#include "graphicresources/imageresourcessvg.h"
#include "tooltips/tooltipcontroller.h"


#include <QDebug>

namespace GuiLocations {

LocationItemCityWidget::LocationItemCityWidget(IWidgetLocationsInfo *widgetLocationsInfo, CityModelItem cityModelItem, QWidget *parent) : SelectableLocationItemWidget(parent)
  , cityModelItem_(cityModelItem)
  , selectable_(false)
  , showingPingBar_(!widgetLocationsInfo->isShowLatencyInMs())
  , widgetLocationsInfo_(widgetLocationsInfo)
{
    cityLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    cityLabel_->setStyleSheet("QLabel { color : white; }");
    cityLabel_->setText(cityModelItem.city);
    cityLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));

    nickLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    nickLabel_->setStyleSheet("QLabel { color : white; }");
    nickLabel_->setText(cityModelItem.nick);
    nickLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));

    pingBarIcon_ = QSharedPointer<IconWidget>(new IconWidget("locations/LOCATION_PING_BARS0", this));
    pingBarIcon_->setOpacity(OPACITY_HALF);
    connect(pingBarIcon_.get(), SIGNAL(hoverEnter()), SLOT(onPingBarIconHoverEnter()));
    connect(pingBarIcon_.get(), SIGNAL(hoverLeave()), SLOT(onPingBarIconHoverLeave()));
    if (widgetLocationsInfo->isShowLatencyInMs()) pingBarIcon_->hide();

    recalcItemPositions();
}

LocationItemCityWidget::~LocationItemCityWidget()
{
    // qDebug() << "Deleting city widget: " << cityModelItem_.city << " " << cityModelItem_.nick;
}

const LocationID LocationItemCityWidget::getId() const
{
    return cityModelItem_.id;
}

const QString LocationItemCityWidget::name() const
{
    return cityModelItem_.city + " " + cityModelItem_.nick;
}

SelectableLocationItemWidget::SelectableLocationItemWidgetType LocationItemCityWidget::type()
{
    return SelectableLocationItemWidgetType::CITY;
}

void LocationItemCityWidget::setSelectable(bool selectable)
{
    selectable_ = selectable;
}

void LocationItemCityWidget::setSelected(bool select)
{
    if (selectable_)
    {
        // Do not filter out same-state changes since it is possible to get locked in an updatable state (though relatively rare)
        selected_ = select;
        if (select)
        {
            //qDebug() << "Selecting City: " << cityLabel_->text() << " " << nickLabel_->text();
            cityLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_FULL));
            nickLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_FULL));
            emit selected(this);
        }
        else
        {
            //qDebug() << "Unselecting City: " << cityLabel_->text() << " " << nickLabel_->text();
            cityLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));
            nickLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));
        }
        update();
    }
}

bool LocationItemCityWidget::isSelected() const
{
    return selected_;
}

bool LocationItemCityWidget::containsCursor() const
{
    const QPoint originAsGlobal = mapToGlobal(QPoint(0,0));
    QRect geoRectAsGlobal(originAsGlobal.x(), originAsGlobal.y(), geometry().width(), geometry().height());
    bool result = geoRectAsGlobal.contains(cursor().pos());
    return result;
}

void LocationItemCityWidget::setLatencyMs(PingTime pingTime)
{
    pingTime_ = pingTime;
}

void LocationItemCityWidget::setShowLatencyMs(bool showLatencyMs)
{
    // ms drawing handled in paintEvent
    if (showLatencyMs)
    {
        pingBarIcon_->hide();
    }
    else
    {
        pingBarIcon_->show();
    }
    showingPingBar_ = !showLatencyMs;
    update();
}

void LocationItemCityWidget::paintEvent(QPaintEvent *event)
{
    // background
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE),
                      WidgetLocationsSizes::instance().getBackgroundColor());

    // pro star
    if (cityModelItem_.bShowPremiumStarOnly && widgetLocationsInfo_->isFreeSessionStatus())
    {
        IndependentPixmap *premiumStarPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_CITY_STAR");
        premiumStarPixmap->draw(LOCATION_ITEM_MARGIN * G_SCALE,
                                (LOCATION_ITEM_HEIGHT*G_SCALE - premiumStarPixmap->height()) / 2, &painter);
    }

    if (!showingPingBar_)
    {
        // draw bubble
        int latencyRectWidth = 33;
        int latencyRectHeight = 20;
        int scaledX = (WINDOW_WIDTH - latencyRectWidth - 8) * G_SCALE;
        int scaledY = (LOCATION_ITEM_HEIGHT - latencyRectHeight) / 2 *G_SCALE - 1*G_SCALE;
        QRect latencyRect(scaledX, scaledY,
                          latencyRectWidth * G_SCALE, latencyRectHeight *G_SCALE);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QBrush(QColor(40, 45, 61)));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(latencyRect, 4*G_SCALE, 4*G_SCALE);

        // draw latency text
        QFont font = *FontManager::instance().getFont(11, false);
        QString latencyText = QString::number(pingTime_.toInt());
        painter.setBrush(Qt::white);
        painter.setPen(Qt::white);
        painter.setFont(font);
        painter.drawText(QRect((scaledX + latencyRectWidth/2*G_SCALE) - CommonGraphics::textWidth(latencyText, font)/2,
                               (scaledY + latencyRectHeight/2*G_SCALE) - CommonGraphics::textHeight(font)/2 ,
                               CommonGraphics::textWidth(latencyText, font),
                               CommonGraphics::textHeight(font)),
                         latencyText);
    }

    // background line
    int left = 24 * G_SCALE;
    int right = WINDOW_WIDTH * G_SCALE - 8*G_SCALE;
    int bottom = LOCATION_ITEM_HEIGHT-1* G_SCALE;
    QPen pen(QColor(0x29, 0x2E, 0x3E));
    pen.setWidth(1);
    painter.setPen(pen);
    painter.drawLine(left, bottom, right, bottom);
    painter.drawLine(left, bottom - 1, right, bottom - 1);
}

void LocationItemCityWidget::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    // qDebug() << "City entered: " << name();
    setSelected(true); // triggers unselection of other widgets
}


void LocationItemCityWidget::resizeEvent(QResizeEvent *event)
{
    recalcItemPositions();
}

void LocationItemCityWidget::onPingBarIconHoverEnter()
{
    if (showingPingBar_)
    {
        QPoint pt = mapToGlobal(QPoint(pingBarIcon_->geometry().center().x(), pingBarIcon_->geometry().top() - 3*G_SCALE));
        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_PING_TIME);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = QString("%1 Ms").arg(pingTime_.toInt());
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        TooltipController::instance().showTooltipBasic(ti);
    }
}

void LocationItemCityWidget::onPingBarIconHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);

}

const QString LocationItemCityWidget::labelStyleSheetWithOpacity(double opacity)
{
    return "QLabel { color : rgba(255,255,255, " + QString::number(opacity) + "); }";
}

void LocationItemCityWidget::recalcItemPositions()
{
    QFont cityFont = *FontManager::instance().getFont(16, true);
    int cityWidth = CommonGraphics::textWidth(cityLabel_->text(), cityFont);
    int cityX = LOCATION_ITEM_MARGIN * G_SCALE * 2 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE;
    cityLabel_->setFont(cityFont);
    cityLabel_->setGeometry(cityX,
                            (LOCATION_ITEM_HEIGHT * G_SCALE - CommonGraphics::textHeight(cityFont))/2,
                            cityWidth,
                            CommonGraphics::textHeight(cityFont));

    QFont nickFont = *FontManager::instance().getFont(16, false);
    nickLabel_->setFont(nickFont);
    nickLabel_->move(150 * G_SCALE, 10 * G_SCALE);
    nickLabel_->setGeometry(cityX + cityWidth + 16*G_SCALE,
                            (LOCATION_ITEM_HEIGHT * G_SCALE - CommonGraphics::textHeight(nickFont))/2,
                            CommonGraphics::textWidth(nickLabel_->text(), nickFont), CommonGraphics::textHeight(nickFont));

    // ping bar
    int scaledX = (WINDOW_WIDTH - 16 - LOCATION_ITEM_MARGIN) * G_SCALE;
    int scaledY = (LOCATION_ITEM_HEIGHT - 16) / 2 * G_SCALE - 1*G_SCALE;
    QRect latencyRect(scaledX, scaledY,
                      16 * G_SCALE, 16 *G_SCALE);
    pingBarIcon_->setGeometry(latencyRect);
}

}
