#ifndef LOCATIONSTRAYMENUNATIVE_H
#define LOCATIONSTRAYMENUNATIVE_H

#include <QHash>
#include <QMenu>
#include <QVector>
#include "../backend/locationsmodel/locationsmodel.h"

class LocationsTrayMenuNative : public QMenu
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuNative(QWidget *parent = nullptr);

    void setLocationsModel(LocationsModel *locationsModel);

signals:
    void locationSelected(QString locationTitle, int cityIndex);

private slots:
    void onSubmenuActionTriggered(QAction *action);
    void onItemsUpdated(QVector<LocationModelItem*> items);
    void onSessionStatusChanged(bool bFreeSessionStatus);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);

private:
    struct CityDesc
    {
        QString cityName;
        QString locationName;
        bool isPro;
    };
    struct LocationDesc
    {
        QMenu *menu;
        QString title;
        QString countryCode;
        bool isEnabled;
        bool containsAtLeastOneNonProCity;
        QVector<CityDesc> cities;
    };

    void rebuildMenu();

    bool bIsFreeSession_;
    QVector<LocationDesc> locationsDesc_;
    QHash<LocationID, int> locationsMap_;
};

#endif // LOCATIONSTRAYMENUNATIVE_H
