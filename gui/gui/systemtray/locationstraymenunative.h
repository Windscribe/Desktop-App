#ifndef LOCATIONSTRAYMENUNATIVE_H
#define LOCATIONSTRAYMENUNATIVE_H

#include <QHash>
#include <QMenu>
#include <QVector>
#include "locationstraymenutypes.h"
#include "../backend/locationsmodel/locationsmodel.h"

class LocationsTrayMenuNative : public QMenu
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuNative(QWidget *parent = nullptr);

    void setMenuType(LocationsTrayMenuType type);
    void setLocationsModel(LocationsModel *locationsModel);

signals:
    void locationSelected(int type, QString locationTitle, int cityIndex);

private slots:
    void onMenuActionTriggered(QAction *action);
    void onSubmenuActionTriggered(QAction *action);
    void onItemsUpdated(QVector<LocationModelItem*> items);
    void onFavoritesUpdated(QVector<CityModelItem*> items);
    void onStaticIpsUpdated(QVector<CityModelItem*> items);
    void onCustomConfigsUpdated(QVector<CityModelItem*> items);
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
        QString name;
        QString title;
        QString countryCode;
        int flags;
        QVector<CityDesc> cities;
    };

    static constexpr int ITEM_FLAG_IS_ENABLED = 1 << 0;
    static constexpr int ITEM_FLAG_IS_VALID = 1 << 1;
    static constexpr int ITEM_FLAG_IS_PREMIUM_ONLY = 1 << 2;

    void rebuildMenu();

    LocationsTrayMenuType locationType_;
    bool bIsFreeSession_;
    QVector<LocationDesc> locationsDesc_;
    QHash<LocationID, int> locationsMap_;
};

#endif // LOCATIONSTRAYMENUNATIVE_H
