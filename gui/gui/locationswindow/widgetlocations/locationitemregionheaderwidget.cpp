#include "locationitemregionheaderwidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "widgetlocationssizes.h"

#include <QDebug>

namespace GuiLocations {

LocationItemRegionHeaderWidget::LocationItemRegionHeaderWidget(LocationModelItem *locationModelItem, QWidget *parent) : SelectableLocationItemWidget(parent)
  , locationID_(locationModelItem->id)
  , selected_(false)
{
    // setMouseTracking(true);

    height_ = REGION_HEADER_HEIGHT;
    textLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    textLabel_->setFont(*FontManager::instance().getFont(16, true));
    textLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));
    textLabel_->setText(locationModelItem->title);
    textLabel_->setWindowOpacity(OPACITY_HALF);
    textLabel_->show();

    updateScaling();
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

void LocationItemRegionHeaderWidget::setSelected(bool select)
{
    if (selected_ != select)
    {
        selected_ = select;
        if (select)
        {
            qDebug() << "Selecting Region: " << textLabel_->text();
            textLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_FULL));
            emit selected(this);
        }
        else
        {
            qDebug() << "Unselecting Region: " << textLabel_->text();
            textLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));
        }
    }
    update();
}

bool LocationItemRegionHeaderWidget::isSelected() const
{
    return selected_;
}

void LocationItemRegionHeaderWidget::updateScaling()
{
    textLabel_->setFont(*FontManager::instance().getFont(16, true));
    textLabel_->move(10 * G_SCALE, 10 * G_SCALE);
}

void LocationItemRegionHeaderWidget::paintEvent(QPaintEvent *event)
{
    // qDebug() << "Region painting";
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, height_ * G_SCALE),
                     WidgetLocationsSizes::instance().getBackgroundColor());
}

void LocationItemRegionHeaderWidget::enterEvent(QEvent *event)
{
    setSelected(true); // triggers unselection of other widgets
}

const QString LocationItemRegionHeaderWidget::labelStyleSheetWithOpacity(double opacity)
{
    return "QLabel { color : rgba(255,255,255, " + QString::number(opacity) + "); }";
}

LocationID LocationItemRegionHeaderWidget::getId()
{
    return locationID_;
}

} // namespace
