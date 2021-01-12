#ifndef LOCATIONITEMREGIONWIDGET_H
#define LOCATIONITEMREGIONWIDGET_H

#include <QAbstractButton>
#include <QList>
#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "citynode.h"
#include "locationitemcitywidget.h"

namespace GuiLocations {

class LocationItemRegionWidget : public QAbstractButton
{
    Q_OBJECT
public:
    explicit LocationItemRegionWidget(LocationModelItem *locationItem, QWidget *parent = nullptr);
    ~LocationItemRegionWidget();

    LocationID getId();
    const QString name() const;
    bool expandable() const;
    bool expanded() const;
    void setExpanded(bool expand);
    void setShowLatencyMs(bool showLatencyMs);

    void addCity(CityModelItem city);
    void updateScaling();

    void recalcItemPos();

    static const int REGION_HEIGHT = 50;

signals:
    void heightChanged(int height);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool expanded_;
    int height_;
    QSharedPointer<QLabel> textLabel_;
    LocationID locationID_;
    QList<QSharedPointer<LocationItemCityWidget>> cities_;
};

}

#endif // LOCATIONITEMREGIONWIDGET_H
