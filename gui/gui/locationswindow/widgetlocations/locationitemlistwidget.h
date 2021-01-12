#ifndef LOCATIONITEMLISTWIDGET_H
#define LOCATIONITEMLISTWIDGET_H

#include <QWidget>
#include "locationitemregionwidget.h"

namespace GuiLocations {

class LocationItemListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationItemListWidget(QWidget *parent = nullptr);
    ~LocationItemListWidget();

    void clearWidgets();
    void addRegionWidget(LocationModelItem *item);
    void addCityToRegion(CityModelItem city, LocationModelItem *region);

    void updateScaling();

signals:
    void heightChanged(int height);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private slots:
    void onRegionWidgetHeightChanged(int height);
    void onRegionWidgetClicked();

private:
    int height_;
    QList<QSharedPointer<LocationItemRegionWidget>> itemWidgets_;
    void recalcItemPos();
};

}

#endif // LOCATIONITEMLISTWIDGET_H
