#include "itemwidgetheader.h"

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

ItemWidgetHeader::ItemWidgetHeader(IWidgetLocationsInfo *widgetLocationsInfo, LocationModelItem *locationModelItem, QWidget *parent) : IItemWidget(parent)
  , locationID_(locationModelItem->id)
  , countryCode_(locationModelItem->countryCode)
  , isPremiumOnly_(locationModelItem->isPremiumOnly)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , showPlusIcon_(true)
  , plusIconOpacity_(OPACITY_THIRD)
  , textOpacity_(OPACITY_UNHOVER_TEXT)
  , expandAnimationProgress_(0.0)
  , expanded_(false)
  , selected_(false)
  , selectable_(true)
{
    // setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);

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

ItemWidgetHeader::~ItemWidgetHeader()
{
    // qDebug() << "Deleting header: " << name();
}

bool ItemWidgetHeader::isExpanded() const
{
    return expanded_;
}

bool ItemWidgetHeader::isForbidden() const
{
    return false;
}

bool ItemWidgetHeader::isDisabled() const
{
    return false;
}

const QString ItemWidgetHeader::name() const
{
    return textLabel_->text();
}

IItemWidget::ItemWidgetType ItemWidgetHeader::type()
{
    return ItemWidgetType::REGION;
}

bool ItemWidgetHeader::containsCursor() const
{
    return globalGeometry().contains(cursor().pos());
}

QRect ItemWidgetHeader::globalGeometry() const
{
    const  QPoint originAsGlobal = mapToGlobal(QPoint(0,0));
    return QRect(originAsGlobal.x(), originAsGlobal.y(), geometry().width(), geometry().height());
}

void ItemWidgetHeader::setSelectable(bool selectable)
{
    selectable_ = selectable;

}

void ItemWidgetHeader::setSelected(bool select)
{
    if (selected_ != select)
    {
        selected_ = select;
        if (select)
        {
            opacityAnimation_.setDirection(QAbstractAnimation::Forward);
            emit selected();
        }
        else
        {
            opacityAnimation_.setDirection(QAbstractAnimation::Backward);
        }
        opacityAnimation_.start();
    }
}

bool ItemWidgetHeader::isSelected() const
{
    return selected_;
}

void ItemWidgetHeader::setExpanded(bool expand)
{
    if (expand)
    {
        expandAnimation_.setDirection(QAbstractAnimation::Forward);
    }
    else
    {
        expandAnimation_.setDirection(QAbstractAnimation::Backward);
    }
    expanded_ = expand;
    expandAnimation_.start();
}

void ItemWidgetHeader::setExpandedWithoutAnimation(bool expand)
{
    if (expand)
    {
        expandAnimationProgress_ = 1.0;
    }
    else
    {
        expandAnimationProgress_ = 0.0;
    }
    expanded_ = expand;
    update();
}

void ItemWidgetHeader::paintEvent(QPaintEvent *event)
{

    // qDebug() << "Header paintEvent";

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
    if (widgetLocationsInfo_->isFreeSessionStatus() && isPremiumOnly_)
    {
        IndependentPixmap *proRegionStar = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_REGION_STAR_LIGHT");
        proRegionStar->draw(8 * G_SCALE,  (LOCATION_ITEM_HEIGHT*G_SCALE - 16*G_SCALE) / 2 - 9*G_SCALE, &painter);
    }

    // plus/cross
    if (!locationID_.isBestLocation())
    {
        painter.setOpacity(plusIconOpacity_);
        IndependentPixmap *expandPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");

        // this part is kind of magical - could use some more clear math
        painter.save();
        painter.translate(QPoint((WINDOW_WIDTH - LOCATION_ITEM_MARGIN)*G_SCALE - expandPixmap->width()/2, LOCATION_ITEM_HEIGHT*G_SCALE/2));
        painter.rotate(45 * expandAnimationProgress_);
        expandPixmap->draw(-expandPixmap->width() / 2,
                           -expandPixmap->height()/ 2,
                   &painter);
        painter.restore();
    }

    int left = 24 * G_SCALE;
    int right = WINDOW_WIDTH * G_SCALE - 8*G_SCALE;
    int bottom = (LOCATION_ITEM_HEIGHT-1)* G_SCALE;
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

void ItemWidgetHeader::enterEvent(QEvent *event)
{
    Q_UNUSED(event)
    // qDebug() << "Selection by hover enter";
    setSelected(true); // triggers unselection of other widgets
}

void ItemWidgetHeader::resizeEvent(QResizeEvent *event)
{
    // qDebug() << "Header resize";
    recalcItemPositions();
}

void ItemWidgetHeader::onP2pIconHoverEnter()
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

void ItemWidgetHeader::onP2pIconHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
}

void ItemWidgetHeader::onOpacityAnimationValueChanged(const QVariant &value)
{
    plusIconOpacity_ = OPACITY_THIRD + (value.toDouble() * (1-OPACITY_THIRD));
    textOpacity_ = OPACITY_UNHOVER_TEXT + (value.toDouble() * (1-OPACITY_UNHOVER_TEXT));

    textLabel_->setStyleSheet(labelStyleSheetWithOpacity(textOpacity_));
    update();
}

void ItemWidgetHeader::onExpandRotationAnimationValueChanged(const QVariant &value)
{
    expandAnimationProgress_ = value.toDouble();
    update();
}

const QString ItemWidgetHeader::labelStyleSheetWithOpacity(double opacity)
{
    return "QLabel { color : rgba(255,255,255, " + QString::number(opacity) + "); }";
}

void ItemWidgetHeader::recalcItemPositions()
{
    // qDebug() << "Header recalc item positions";
    QFont font = *FontManager::instance().getFont(16, true);
    textLabel_->setFont(font);
    textLabel_->setGeometry(LOCATION_ITEM_MARGIN * G_SCALE * 2 + LOCATION_ITEM_FLAG_WIDTH * G_SCALE,
                            (LOCATION_ITEM_HEIGHT * G_SCALE - CommonGraphics::textHeight(font))/2,
                            CommonGraphics::textWidth(textLabel_->text(), font), CommonGraphics::textHeight(font));

    p2pIcon_->setGeometry((WINDOW_WIDTH - 65)*G_SCALE,
                          (LOCATION_ITEM_HEIGHT*G_SCALE - p2pIcon_->height())/2,
                          p2pIcon_->width(), p2pIcon_->height());
}

const LocationID ItemWidgetHeader::getId() const
{
    return locationID_;
}

} // namespace
