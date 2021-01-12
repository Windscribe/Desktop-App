#ifndef LOCATIONITEMREGIONWIDGET_H
#define LOCATIONITEMREGIONWIDGET_H

#include <QWidget>
#include <QList>
#include <QLabel>
#include "locationitem.h"
#include "citynode.h"
#include "locationitemcitywidget.h"

namespace GuiLocations {

class LocationItemRegionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationItemRegionWidget(LocationItem *locationItem, QWidget *parent = nullptr);
    explicit LocationItemRegionWidget(QWidget *parent = nullptr);

    void setExpanded(bool expand);
    void setShowLatencyMs(bool showLatencyMs);

    void setRegion(LocationItem *locationItem);
    void setCities(QList<QSharedPointer<CityNode> > cities);
    void updateScaling();

    void recalcItemPos();

signals:
    void heightChanged(int height);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool expanded_;
    int height_;
    const int REGION_HEIGHT = 50;
    LocationItem *locationItem_;
    QLabel textLabel_;
    QList<QSharedPointer<LocationItemCityWidget>> cities_;
};

}

#endif // LOCATIONITEMREGIONWIDGET_H
