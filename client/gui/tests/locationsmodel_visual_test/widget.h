#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "../../locations/model/locationsmodel.h"
#include "../../locations/view/locationsview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();


private:
    Ui::Widget *ui;

    gui_locations::LocationsModel *locationsModel_;

    QVector<types::Location> currentLocations_;
    QVector<types::Location> testOriginal_;
    QVector<types::Location> testDeletedLocations_;
    QVector<types::Location> testDeletedCities_;
    QVector<types::Location> testChangedLocationsCaptions_;
    QVector<types::Location> testChangedCitiesCaptions_;
    QVector<types::Location> testChangedLocationsOrder_;
    QVector<types::Location> testChangedCitiesOrder_;

    QVector<types::Location> testSimple1_;
    QVector<types::Location> testSimple2_;

    types::Location customConfigLocation_;
    types::Location customConfigLocation2_;

    gui_locations::LocationsView *locationsView_;

    void updateLocations(const LocationID &bestLocation, const QVector<types::Location> &newLocations);
    QVector<types::Location> generateRandomLocations();
};
#endif // WIDGET_H
