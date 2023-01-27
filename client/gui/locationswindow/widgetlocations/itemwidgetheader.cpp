#include "itemwidgetheader.h"

#include <QPainter>
#include <QtMath>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"

#include <QDebug>

namespace GuiLocations {

ItemWidgetHeader::ItemWidgetHeader(IWidgetLocationsInfo *widgetLocationsInfo, LocationModelItem *locationModelItem, QWidget *parent) : IItemWidget(parent)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , locationID_(locationModelItem->id)
  , countryCode_(locationModelItem->countryCode)
  , isPremiumOnly_(locationModelItem->isPremiumOnly)
  , showPlusIcon_(true)
  , accented_(false)
  , selectable_(true)
  , show10gbpsIcon_(locationModelItem->is10gbps)
  , locationLoad_(locationModelItem->locationLoad)
  , plusIconOpacity_(OPACITY_THIRD)
  , expanded_(false)
  , expandAnimationProgress_(0.0)
  , textOpacity_(OPACITY_UNHOVER_TEXT)
  , textForLayout_(locationModelItem->title)
  , textLayout_(nullptr)
  , showP2pIcon_(locationModelItem->isShowP2P)
  , p2pHovering_(false)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);

    recreateTextLayout();

    opacityAnimation_.setStartValue(0.0);
    opacityAnimation_.setEndValue(1.0);
    opacityAnimation_.setDuration(200);
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOpacityAnimationValueChanged(QVariant)));

    expandAnimation_.setStartValue(0.0);
    expandAnimation_.setEndValue(1.0);
    expandAnimation_.setDuration(200);
    connect(&expandAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandRotationAnimationValueChanged(QVariant)));
}

ItemWidgetHeader::~ItemWidgetHeader()
{
    // qDebug() << "Deleting header: " << name();
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);

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

bool ItemWidgetHeader::isBrokenConfig() const
{
    return false;
}

const QString ItemWidgetHeader::name() const
{
    return textForLayout_;
}

IItemWidget::ItemWidgetType ItemWidgetHeader::type()
{
    return ItemWidgetType::REGION;
}

bool ItemWidgetHeader::containsCursor() const
{
    return globalGeometry().contains(cursor().pos());
}

bool ItemWidgetHeader::containsGlobalPoint(const QPoint &pt)
{
    return globalGeometry().contains(pt);
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

void ItemWidgetHeader::setAccented(bool accent)
{
    if (accented_ != accent)
    {
        accented_ = accent;
        if (accent)
        {
            opacityAnimation_.setDirection(QAbstractAnimation::Forward);
            emit accented();
        }
        else
        {
            opacityAnimation_.setDirection(QAbstractAnimation::Backward);
        }
        opacityAnimation_.start();
    }
}

void ItemWidgetHeader::setAccentedWithoutAnimation(bool accent)
{
    opacityAnimation_.stop();
    if (accented_ != accent)
    {
        accented_ = accent;
        if (accent)
        {
            plusIconOpacity_ = OPACITY_FULL;
            textOpacity_ = OPACITY_FULL;
            emit accented();
        }
        else
        {
            plusIconOpacity_ = OPACITY_THIRD;
            textOpacity_ = OPACITY_UNHOVER_TEXT;
        }
    }
    update();
}

bool ItemWidgetHeader::isAccented() const
{
    return accented_;
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

void ItemWidgetHeader::updateScaling()
{
    recreateTextLayout();
}

void ItemWidgetHeader::paintEvent(QPaintEvent * /*event*/)
{
    // background
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE),
                     FontManager::instance().getMidnightColor());

    // flag
    QSharedPointer<IndependentPixmap> flag = ImageResourcesSvg::instance().getFlag(countryCode_);
    if (flag)
    {
        const int pixmapFlagHeight = flag->height();
        flag->draw(LOCATION_ITEM_MARGIN*G_SCALE,  (LOCATION_ITEM_HEIGHT*G_SCALE - pixmapFlagHeight) / 2, &painter);
    }

    // pro star
    if (widgetLocationsInfo_->isFreeSessionStatus() && isPremiumOnly_)
    {
        QSharedPointer<IndependentPixmap> proRegionStar = ImageResourcesSvg::instance().getIndependentPixmap("locations/PRO_REGION_STAR_LIGHT");
        proRegionStar->draw(8 * G_SCALE,  (LOCATION_ITEM_HEIGHT*G_SCALE - 16*G_SCALE) / 2 - 9*G_SCALE, &painter);
    }

    // text
    painter.save();
    painter.setOpacity(textOpacity_);
    painter.setPen(Qt::white);
    textLayout_->draw(&painter, QPoint(textLayoutRect().x(), textLayoutRect().y()));
    painter.restore();

    // p2p
    if (showP2pIcon_)
    {
        painter.setOpacity(OPACITY_HALF);

        QRect p2pr = p2pRect();
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");
        if (p) p->draw(p2pr.x(),p2pr.y(),&painter);
    }

    if (locationID_.isBestLocation())
    {
        if (show10gbpsIcon_)
        {
            painter.setOpacity(OPACITY_FULL);
            QSharedPointer<IndependentPixmap> tenGbpsPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/10_GBPS_ICON");

            // this part is kind of magical - could use some more clear math
            painter.save();
            painter.translate(QPoint((WINDOW_WIDTH - LOCATION_ITEM_MARGIN)*G_SCALE - tenGbpsPixmap->width()/2, LOCATION_ITEM_HEIGHT*G_SCALE/2));
            tenGbpsPixmap->draw(-tenGbpsPixmap->width() / 2, -tenGbpsPixmap->height()/ 2, &painter);
            painter.restore();
        }
    }
    else
    {
        // plus/cross
        painter.setOpacity(plusIconOpacity_);
        QSharedPointer<IndependentPixmap> expandPixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/EXPAND_ICON");

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
    int bottom = static_cast<int>(LOCATION_ITEM_HEIGHT*G_SCALE) - 1; // 1 is not scaled since we want bottom-most pixel inside geometry
    painter.setOpacity(1.0);

    // TODO: lines not scaled since we draw just single pixels
    // background line (darker line)
    QPen pen(QColor(0x29, 0x2E, 0x3E));
    pen.setWidth(1);
    painter.setPen(pen);
    painter.drawLine(left, bottom - 1, right, bottom - 1);
    painter.drawLine(left, bottom, right, bottom);

    if (widgetLocationsInfo_->isShowLocationLoad() && locationLoad_ > 0)
    {
        Qt::GlobalColor penColor;
        if (locationLoad_ < 60) {
            penColor = Qt::green;
        }
        else if (locationLoad_ < 90) {
            penColor = Qt::yellow;
        }
        else {
            penColor = Qt::red;
        }
        int rightX = left + ((right - left) * locationLoad_ / 100);
        QPen penLoad(penColor);
        penLoad.setWidth(1);
        painter.setOpacity(textOpacity_);
        painter.setPen(penLoad);
        painter.drawLine(left, bottom - 1, rightX, bottom - 1);
        painter.drawLine(left, bottom, rightX, bottom);
        painter.setOpacity(1.0);
    }

    // top-most line (white)
    if( qFabs(1.0 - expandAnimationProgress_) < 0.000001 )
    {
        QPen white_pen(Qt::white);
        white_pen.setWidth(1);
        painter.setPen(white_pen);
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

void ItemWidgetHeader::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    // qDebug() << "Selection by hover enter";
    emit hoverEnter();
}

void ItemWidgetHeader::leaveEvent(QEvent *event)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
    QWidget::leaveEvent(event);
}

void ItemWidgetHeader::mouseMoveEvent(QMouseEvent *event)
{
    // qDebug() << "Mouse move: " << event->pos();
    if (showP2pIcon_)
    {
        if (p2pRect().contains(event->pos()))
        {
            // qDebug() << "P2P hovering";
            p2pHovering_ = true;
            p2pIconHover();
        }
        else if (p2pHovering_)
        {
            p2pHovering_ = false;
            // qDebug() << "P2P unhover";
            p2pIconUnhover();
        }
    }
}

void ItemWidgetHeader::p2pIconHover()
{
    // p2p tooltip
    QRect r = p2pRect();
    QPoint pt = mapToGlobal(QPoint(r.center().x(), r.top() - 3*G_SCALE));
    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_P2P);
    ti.x = pt.x();
    ti.y = pt.y();
    ti.title = tr("File Sharing Frowned Upon");
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    TooltipController::instance().showTooltipBasic(ti);
}

void ItemWidgetHeader::p2pIconUnhover()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
}

void ItemWidgetHeader::onOpacityAnimationValueChanged(const QVariant &value)
{
    plusIconOpacity_ = OPACITY_THIRD + (value.toDouble() * (1-OPACITY_THIRD));
    textOpacity_ = OPACITY_UNHOVER_TEXT + (value.toDouble() * (1-OPACITY_UNHOVER_TEXT));

    update();
}

void ItemWidgetHeader::onExpandRotationAnimationValueChanged(const QVariant &value)
{
    expandAnimationProgress_ = value.toDouble();
    update();
}

QFont ItemWidgetHeader::textLayoutFont()
{
    return *FontManager::instance().getFont(16, true);
}

QRect ItemWidgetHeader::textLayoutRect()
{
    QFont f = textLayoutFont();
    return QRect(64*G_SCALE,
                 (LOCATION_ITEM_HEIGHT*G_SCALE - CommonGraphics::textHeight(f))/2,
                 CommonGraphics::textWidth(textForLayout_, f),
                 CommonGraphics::textHeight(f));
}

void ItemWidgetHeader::recreateTextLayout()
{
    textLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(textForLayout_, textLayoutFont()));
    textLayout_->beginLayout();
    textLayout_->createLine();
    textLayout_->endLayout();
    textLayout_->setCacheEnabled(true);
}

QRect ItemWidgetHeader::p2pRect()
{
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("locations/NO_P2P_ICON");

    return QRect((WINDOW_WIDTH - 65)*G_SCALE,
                 (LOCATION_ITEM_HEIGHT*G_SCALE - p->height())/2,
                 p->width(),
                 p->height());
}

const LocationID ItemWidgetHeader::getId() const
{
    return locationID_;
}

} // namespace
