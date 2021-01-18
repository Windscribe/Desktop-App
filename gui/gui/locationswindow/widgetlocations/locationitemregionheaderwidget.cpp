#include "locationitemregionheaderwidget.h"

#include <QPainter>
#include <QtMath>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "widgetlocationssizes.h"
#include "graphicresources/imageresourcessvg.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"

#include <QDebug>

namespace GuiLocations {

LocationItemRegionHeaderWidget::LocationItemRegionHeaderWidget(IWidgetLocationsInfo *widgetLocationsInfo, LocationModelItem *locationModelItem, QWidget *parent) : SelectableLocationItemWidget(parent)
  , locationID_(locationModelItem->id)
  , countryCode_(locationModelItem->countryCode)
  , isPremiumOnly_(locationModelItem->isPremiumOnly)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , showPlusIcon_(true)
  , plusIconOpacity_(OPACITY_THIRD)
  , textOpacity_(OPACITY_UNHOVER_TEXT)
  , expandAnimationProgress_(0.0)
  , selected_(false)
  , selectable_(true)
{
    // setMouseTracking(true);

    textLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    textLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));
    textLabel_->setText(locationModelItem->title);
    textLabel_->show();

    p2pIcon_ = QSharedPointer<IconWidget>(new IconWidget("locations/NO_P2P_ICON", this));
    p2pIcon_->setOpacity(OPACITY_HALF);
    connect(p2pIcon_.get(), SIGNAL(hoverEnter()), SLOT(onP2pIconHoverEnter()));
    connect(p2pIcon_.get(), SIGNAL(hoverLeave()), SLOT(onP2pIconHoverLeave()));
    p2pIcon_->hide();
    if (locationModelItem->isShowP2P) p2pIcon_->show();

    opacityAnimation_.setStartValue(0.0);
    opacityAnimation_.setEndValue(1.0);
    opacityAnimation_.setDuration(200);
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOpacityAnimationValueChanged(QVariant)));

    expandAnimation_.setStartValue(0.0);
    expandAnimation_.setEndValue(1.0);
    expandAnimation_.setDuration(200);
    connect(&expandAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandRotationAnimationValueChanged(QVariant)));

    recalcItemPositions();
}

const QString LocationItemRegionHeaderWidget::name() const
{
    return textLabel_->text();
}

SelectableLocationItemWidget::SelectableLocationItemWidgetType LocationItemRegionHeaderWidget::type()
{
    return SelectableLocationItemWidgetType::REGION;
}

bool LocationItemRegionHeaderWidget::containsCursor() const
{
    const QPoint originAsGlobal = mapToGlobal(QPoint(0,0));
    QRect geoRectAsGlobal(originAsGlobal.x(), originAsGlobal.y(), geometry().width(), geometry().height());
    bool result = geoRectAsGlobal.contains(cursor().pos());
    return result;
}

void LocationItemRegionHeaderWidget::setSelectable(bool selectable)
{
    selectable_ = selectable;

}

void LocationItemRegionHeaderWidget::setSelected(bool select)
{
    if (selected_ != select)
    {
        selected_ = select;
        if (select)
        {
            //qDebug() << "Selecting Region: " << textLabel_->text();
            opacityAnimation_.setDirection(QAbstractAnimation::Forward);
            emit selected(this);
        }
        else
        {
            //qDebug() << "Unselecting Region: " << textLabel_->text();
            opacityAnimation_.setDirection(QAbstractAnimation::Backward);
        }
        opacityAnimation_.start();
    }
}

bool LocationItemRegionHeaderWidget::isSelected() const
{
    return selected_;
}

void LocationItemRegionHeaderWidget::showPlusIcon(bool show)
{
    showPlusIcon_ = show;
    update();
}

void LocationItemRegionHeaderWidget::setExpanded(bool expand)
{
    if (expand)
    {
        expandAnimation_.setDirection(QAbstractAnimation::Forward);
    }
    else
    {
        expandAnimation_.setDirection(QAbstractAnimation::Backward);
    }
    expandAnimation_.start();
}

void LocationItemRegionHeaderWidget::paintEvent(QPaintEvent *event)
{
    // qDebug() << "Region painting";

    // background
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE),
                     WidgetLocationsSizes::instance().getBackgroundColor());

    // flag
    IndependentPixmap *flag = ImageResourcesSvg::instance().getFlag(countryCode_);
    if (flag)
    {
        const int pixmapFlagHeight = flag->height();
        flag->draw(LOCATION_ITEM_MARGIN*G_SCALE,  (LOCATION_ITEM_HEIGHT*G_SCALE - pixmapFlagHeight) / 2, &painter);
    }

    // pro star
    qDebug() << "free status : " << widgetLocationsInfo_->isFreeSessionStatus();
    qDebug() << "premium: " << isPremiumOnly_;
    if (widgetLocationsInfo_->isFreeSessionStatus() && isPremiumOnly_)
    {
        qDebug() << "Drawing star";
        IndependentPixmap *proRegionStar = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_REGION_STAR_LIGHT");
        proRegionStar->draw(8 * G_SCALE,  (LOCATION_ITEM_HEIGHT*G_SCALE - 16*G_SCALE) / 2 - 9*G_SCALE, &painter);
    }

    // plus/cross or construction icon
    if (!locationID_.isBestLocation())
    {
        painter.setOpacity(plusIconOpacity_);
        if (showPlusIcon_)
        {
            IndependentPixmap *expandPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");

            // this part is kind of magical - could use some more clear math
            painter.save();
            painter.translate(QPoint((WINDOW_WIDTH - LOCATION_ITEM_MARGIN)*G_SCALE - expandPixmap->width()/2, LOCATION_ITEM_HEIGHT/2));
            painter.rotate(45 * expandAnimationProgress_);
            expandPixmap->draw(-expandPixmap->width() / 2,
                               -expandPixmap->height()/ 2,
                       &painter);
            painter.restore();
        }
        else
        {
            IndependentPixmap *consIcon = ImageResourcesSvg::instance().getIndependentPixmap("locations/UNDER_CONSTRUCTION_ICON");
            if (consIcon)
            {
                consIcon->draw((WINDOW_WIDTH - LOCATION_ITEM_MARGIN)*G_SCALE - consIcon->width(),
                           (LOCATION_ITEM_HEIGHT*G_SCALE - consIcon->height()) / 2,
                           &painter);
            }
        }
    }

    int left = 24 * G_SCALE;
    int right = WINDOW_WIDTH * G_SCALE - left;
    int bottom = LOCATION_ITEM_HEIGHT-1* G_SCALE;
    painter.setOpacity(1.0);

    // background line (darker line)
    QPen pen(QColor(0x29, 0x2E, 0x3E));
    pen.setWidth(1);
    painter.setPen(pen);
    painter.drawLine(left, bottom, right, bottom);
    painter.drawLine(left, bottom - 1, right, bottom - 1);

    // top-most line (white)
    if( qFabs(1.0 - expandAnimationProgress_) < 0.000001 )
    {
        QPen pen(Qt::white);
        pen.setWidth(1);
        painter.setPen(pen);
        painter.drawLine(left, bottom, right, bottom);
        painter.drawLine(left, bottom - 1, right, bottom - 1);
    }
    else if (expandAnimationProgress_ > 0.000001)
    {
        int w = (right - left) * expandAnimationProgress_;
        QPen white_pen(Qt::white);
        white_pen.setWidth(1);
        painter.setPen(white_pen);
        painter.drawLine(left, bottom, left + w, bottom);
        painter.drawLine(left, bottom - 1, left + w, bottom - 1);
    }
}

void LocationItemRegionHeaderWidget::enterEvent(QEvent *event)
{
    //qDebug() << "Entering header: " << name();
    setSelected(true); // triggers unselection of other widgets
}

void LocationItemRegionHeaderWidget::resizeEvent(QResizeEvent *event)
{
    recalcItemPositions();
}

void LocationItemRegionHeaderWidget::onP2pIconHoverEnter()
{
    // p2p tooltip
    QPoint pt = mapToGlobal(QPoint(p2pIcon_->geometry().center().x(), p2pIcon_->geometry().top() - 3*G_SCALE));
    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_P2P);
    ti.x = pt.x();
    ti.y = pt.y();
    ti.title = tr("File Sharing Frowned Upon");
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    TooltipController::instance().showTooltipBasic(ti);
}

void LocationItemRegionHeaderWidget::onP2pIconHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
}

void LocationItemRegionHeaderWidget::onOpacityAnimationValueChanged(const QVariant &value)
{
    plusIconOpacity_ = OPACITY_THIRD + (value.toDouble() * (1-OPACITY_THIRD));
    textOpacity_ = OPACITY_UNHOVER_TEXT + (value.toDouble() * (1-OPACITY_UNHOVER_TEXT));

    textLabel_->setStyleSheet(labelStyleSheetWithOpacity(textOpacity_));
    update();
}

void LocationItemRegionHeaderWidget::onExpandRotationAnimationValueChanged(const QVariant &value)
{
    expandAnimationProgress_ = value.toDouble();
    update();
}

const QString LocationItemRegionHeaderWidget::labelStyleSheetWithOpacity(double opacity)
{
    return "QLabel { color : rgba(255,255,255, " + QString::number(opacity) + "); }";
}

void LocationItemRegionHeaderWidget::recalcItemPositions()
{
    QFont font = *FontManager::instance().getFont(16, true);
    textLabel_->setFont(font);
    textLabel_->setGeometry(LOCATION_ITEM_MARGIN * G_SCALE * 2 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE,
                            (LOCATION_ITEM_HEIGHT * G_SCALE - CommonGraphics::textHeight(font))/2,
                            CommonGraphics::textWidth(textLabel_->text(), font), CommonGraphics::textHeight(font));

    p2pIcon_->setGeometry((WINDOW_WIDTH - 65)*G_SCALE,
                          (LOCATION_ITEM_HEIGHT*G_SCALE - p2pIcon_->height())/2,
                          p2pIcon_->width(), p2pIcon_->height());
}

const LocationID LocationItemRegionHeaderWidget::getId() const
{
    return locationID_;
}

} // namespace
