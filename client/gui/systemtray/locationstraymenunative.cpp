#include "locationstraymenunative.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

LocationsTrayMenuNative::LocationsTrayMenuNative(QWidget *parent) :
    QMenu(parent)
  , locationType_(LOCATIONS_TRAY_MENU_TYPE_GENERIC)
  , bIsFreeSession_(false)
{
}

void LocationsTrayMenuNative::setMenuType(LocationsTrayMenuType type)
{
    locationType_ = type;
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_GENERIC)
        connect(this, SIGNAL(triggered(QAction*)), this, SLOT(onMenuActionTriggered(QAction*)));
    else
        disconnect(this, SIGNAL(triggered(QAction*)), this, SLOT(onMenuActionTriggered(QAction*)));
}

void LocationsTrayMenuNative::setLocationsModel(LocationsModel *locationsModel)
{
    connect(locationsModel->getAllLocationsModel(), SIGNAL(itemsUpdated(QVector<LocationModelItem*>)), SLOT(onItemsUpdated(QVector<LocationModelItem *>)));
    connect(locationsModel->getAllLocationsModel(), SIGNAL(connectionSpeedChanged(LocationID,PingTime)), SLOT(onConnectionSpeedChanged(LocationID,PingTime)));
    connect(locationsModel->getAllLocationsModel(), SIGNAL(freeSessionStatusChanged(bool)), SLOT(onSessionStatusChanged(bool)));
    connect(locationsModel->getFavoriteLocationsModel(), SIGNAL(itemsUpdated(QVector<CityModelItem*>)), SLOT(onFavoritesUpdated(QVector<CityModelItem *>)));
    connect(locationsModel->getStaticIpsLocationsModel(), SIGNAL(itemsUpdated(QVector<CityModelItem*>)), SLOT(onStaticIpsUpdated(QVector<CityModelItem *>)));
    connect(locationsModel->getConfiguredLocationsModel(), SIGNAL(itemsUpdated(QVector<CityModelItem*>)), SLOT(onCustomConfigsUpdated(QVector<CityModelItem *>)));
}

void LocationsTrayMenuNative::onMenuActionTriggered(QAction *action)
{
    Q_ASSERT(action);
    if (!action || !action->isEnabled())
        return;
    emit locationSelected(locationType_, action->whatsThis(), -1);
}

void LocationsTrayMenuNative::onSubmenuActionTriggered(QAction *action)
{
    Q_ASSERT(action);
    if (!action || !action->isEnabled())
        return;
    const auto *menu = qobject_cast<QMenu*>(action->parent());
    if (!menu || !menu->isEnabled())
        return;
    emit locationSelected(locationType_, action->whatsThis(), menu->actions().indexOf(action));
}

void LocationsTrayMenuNative::onItemsUpdated(QVector<LocationModelItem *> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_GENERIC)
        return;

    locationsMap_.clear();
    locationsDesc_.clear();
    locationsDesc_.reserve(items.count());

    for (const LocationModelItem *item : qAsConst(items)) {
        if (item->id.isCustomConfigsLocation() || item->title.isEmpty() || item->cities.isEmpty())
            continue;

        LocationDesc desc;
        desc.menu = nullptr;
        desc.name = item->title;
        desc.title = item->title;
        desc.countryCode = item->countryCode;
        desc.flags = ITEM_FLAG_IS_VALID | ITEM_FLAG_IS_PREMIUM_ONLY;
        desc.cities.resize(item->cities.count());

        const QVector<CityModelItem> cities = item->cities;
        for (const CityModelItem &city : cities) {
            if (!city.bShowPremiumStarOnly) {
                desc.flags &= ~ITEM_FLAG_IS_PREMIUM_ONLY;
                break;
            }
        }
        const auto connectionSpeed = PingTime(item->calcAveragePing()).toConnectionSpeed();
        if (connectionSpeed != 0)
            desc.flags |= ITEM_FLAG_IS_ENABLED;

        for (int i = 0; i < cities.count(); ++i) {
            auto &city = cities[i];
            desc.cities[i].cityName = city.makeTitle();
            desc.cities[i].locationName = desc.title;
            desc.cities[i].isPro = city.bShowPremiumStarOnly;
        }
        locationsMap_[item->id] = locationsDesc_.count();
        locationsDesc_.push_back(desc);
    }

    rebuildMenu();
}

void LocationsTrayMenuNative::onFavoritesUpdated(QVector<CityModelItem*> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_FAVORITES)
        return;

    locationsMap_.clear();
    locationsDesc_.clear();
    locationsDesc_.reserve(items.count());

    for (const CityModelItem *item : qAsConst(items)) {
        if (item->city.isEmpty())
            continue;

        LocationDesc desc;
        desc.menu = nullptr;
        desc.name = desc.title = item->makeTitle();
        desc.countryCode = item->countryCode;
        desc.flags = ITEM_FLAG_IS_VALID;
        if (item->bShowPremiumStarOnly && bIsFreeSession_) {
            desc.flags |= ITEM_FLAG_IS_PREMIUM_ONLY;
        } else {
            if (item->pingTimeMs != 0)
                desc.flags |= ITEM_FLAG_IS_ENABLED;
        }
        locationsMap_[item->id] = locationsDesc_.count();
        locationsDesc_.push_back(desc);
    }

    rebuildMenu();
}

void LocationsTrayMenuNative::onStaticIpsUpdated(QVector<CityModelItem*> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_STATIC_IPS)
        return;

    locationsMap_.clear();
    locationsDesc_.clear();
    locationsDesc_.reserve(items.count());

    for (const CityModelItem *item : qAsConst(items)) {
        if (item->staticIp.isEmpty())
            continue;

        LocationDesc desc;
        desc.menu = nullptr;
        desc.name = desc.title = item->makeTitle();
        desc.countryCode = item->countryCode;
        desc.flags = ITEM_FLAG_IS_ENABLED | ITEM_FLAG_IS_VALID;
        locationsMap_[item->id] = locationsDesc_.count();
        locationsDesc_.push_back(desc);
    }

    rebuildMenu();
}

void LocationsTrayMenuNative::onCustomConfigsUpdated(QVector<CityModelItem*> items)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_CUSTOM_CONFIGS)
        return;

    locationsMap_.clear();
    locationsDesc_.clear();
    locationsDesc_.reserve(items.count());

    for (const CityModelItem *item : qAsConst(items)) {
        if (item->city.isEmpty())
            continue;
        LocationDesc desc;
        desc.menu = nullptr;
        desc.name = desc.title = item->makeTitle();
        desc.flags = 0;
        if (!item->isDisabled && item->isCustomConfigCorrect)
            desc.flags |= ITEM_FLAG_IS_ENABLED | ITEM_FLAG_IS_VALID;
        locationsMap_[item->id] = locationsDesc_.count();
        locationsDesc_.push_back(desc);
    }

    rebuildMenu();
}

void LocationsTrayMenuNative::onSessionStatusChanged(bool bFreeSessionStatus)
{
    if (bIsFreeSession_ != bFreeSessionStatus) {
        bIsFreeSession_ = bFreeSessionStatus;
        rebuildMenu();
    }
}

void LocationsTrayMenuNative::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    if (locationType_ != LOCATIONS_TRAY_MENU_TYPE_GENERIC &&
        locationType_ != LOCATIONS_TRAY_MENU_TYPE_FAVORITES)
        return;

    auto it = locationsMap_.find(id);
    if (it != locationsMap_.end()) {
        LocationDesc &desc = locationsDesc_[it.value()];
        const bool wasEnabled = !!(desc.flags & ITEM_FLAG_IS_ENABLED);
        const bool isEnabled = (desc.flags & ITEM_FLAG_IS_VALID) && timeMs.toConnectionSpeed() != 0;
        if (wasEnabled != isEnabled) {
            desc.flags |= ITEM_FLAG_IS_ENABLED;
            rebuildMenu();
        }
    }
}

void LocationsTrayMenuNative::rebuildMenu()
{
    clear();

    for (auto &desc : locationsDesc_) {
        desc.menu = nullptr;
        QString itemName = desc.title;
        if ((desc.flags & ITEM_FLAG_IS_PREMIUM_ONLY) && bIsFreeSession_)
            itemName += " (Pro)";

        QSharedPointer<IndependentPixmap> flag;
        if (!desc.countryCode.isEmpty()) {
#if defined(Q_OS_MAC)
            const int flags = ImageResourcesSvg::IMAGE_FLAG_SQUARE;
#else
            const int flags = 0;
#endif
            flag = ImageResourcesSvg::instance().getScaledFlag(
                desc.countryCode, 20 * G_SCALE, 10 * G_SCALE, flags);
        }

        if (desc.cities.count() && 
            (!(desc.flags & ITEM_FLAG_IS_PREMIUM_ONLY) || !bIsFreeSession_)) {
            desc.menu = addMenu(itemName);
            desc.menu->setEnabled(!!(desc.flags & ITEM_FLAG_IS_ENABLED));
            if (flag)
                desc.menu->setIcon(flag->getIcon());
            connect(desc.menu, SIGNAL(triggered(QAction*)),
                    SLOT(onSubmenuActionTriggered(QAction*)));
        } else {
            QAction *action = addAction(itemName);
            action->setEnabled(desc.cities.count() == 0 && (desc.flags & ITEM_FLAG_IS_ENABLED));
            action->setWhatsThis(desc.name);
            if (flag)
                action->setIcon(flag->getIcon());
        }
        if (desc.menu) {
            for (int i = 0; i < desc.cities.count(); ++i) {
                const auto &city = desc.cities[i];
                const bool isActionEnabled = !city.isPro || !bIsFreeSession_;
                QString visibleName = city.cityName;
                if (!isActionEnabled)
                    visibleName += " (Pro)";
                auto *action = desc.menu->addAction(visibleName);
                action->setObjectName(city.cityName);
                action->setWhatsThis(city.locationName);
                action->setEnabled(isActionEnabled);
            }
        }
    }
}
