#include "locationitemcitywidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "widgetlocationssizes.h"


#include <QDebug>

namespace GuiLocations {

LocationItemCityWidget::LocationItemCityWidget(CityModelItem cityModelItem, QWidget *parent) : SelectableLocationItemWidget(parent)
  , cityModelItem_(cityModelItem)
  , selectable_(false)
{
    cityLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    cityLabel_->setStyleSheet("QLabel { color : white; }");
    cityLabel_->setText(cityModelItem.city);
    cityLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));

    nickLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    nickLabel_->setStyleSheet("QLabel { color : white; }");
    nickLabel_->setText(cityModelItem.nick);
    nickLabel_->setStyleSheet(labelStyleSheetWithOpacity(OPACITY_HALF));
    recalcItemPositions();
}

LocationItemCityWidget::~LocationItemCityWidget()
{
    // qDebug() << "Deleting city widget: " << cityModelItem_.city << " " << cityModelItem_.nick;
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
        if (selected_ != select)
        {
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

void LocationItemCityWidget::setShowLatencyMs(bool showLatencyMs)
{

}

void LocationItemCityWidget::paintEvent(QPaintEvent *event)
{
    // background
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, HEIGHT * G_SCALE),
                      WidgetLocationsSizes::instance().getBackgroundColor());

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

const QString LocationItemCityWidget::labelStyleSheetWithOpacity(double opacity)
{
    return "QLabel { color : rgba(255,255,255, " + QString::number(opacity) + "); }";
}

void LocationItemCityWidget::recalcItemPositions()
{
    cityLabel_->setFont(*FontManager::instance().getFont(16, true));
    cityLabel_->move(10 * G_SCALE, 10 * G_SCALE);

    nickLabel_->setFont(*FontManager::instance().getFont(13, false));
    nickLabel_->move(150 * G_SCALE, 10 * G_SCALE);
}

}
