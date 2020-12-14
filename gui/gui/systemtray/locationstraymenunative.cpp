#include "locationstraymenunative.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

LocationsTrayMenuNative::LocationsTrayMenuNative(QWidget *parent) :
    QMenu(parent)
  , bIsFreeSession_(false)
{
}

void LocationsTrayMenuNative::setLocationsModel(LocationsModel *locationsModel)
{
    connect(locationsModel->getAllLocationsModel(), SIGNAL(itemsUpdated(QVector<LocationModelItem*>)), SLOT(onItemsUpdated(QVector<LocationModelItem *>)));
    connect(locationsModel->getAllLocationsModel(), SIGNAL(connectionSpeedChanged(LocationID,PingTime)), SLOT(onConnectionSpeedChanged(LocationID,PingTime)));
    connect(locationsModel->getAllLocationsModel(), SIGNAL(freeSessionStatusChanged(bool)), SLOT(onSessionStatusChanged(bool)));
}

void LocationsTrayMenuNative::onSubmenuActionTriggered(QAction *action)
{
    Q_ASSERT(action);
    if (!action || !action->isEnabled())
        return;
    const auto *menu = qobject_cast<QMenu*>(action->parentWidget());
    if (!menu || !menu->isEnabled())
        return;
    emit locationSelected(action->whatsThis(), menu->actions().indexOf(action));
}

void LocationsTrayMenuNative::onItemsUpdated(QVector<LocationModelItem *> items)
{
    locationsMap_.clear();
    locationsDesc_.clear();
    locationsDesc_.reserve(items.count());

    for (const LocationModelItem *item : qAsConst(items)) {
        if (item->id.isCustomConfigsLocation() || item->title.isEmpty() || item->cities.isEmpty())
            continue;

        LocationDesc desc;
        desc.menu = nullptr;
        desc.title = item->title;
        desc.countryCode = item->countryCode;
        desc.isEnabled = false;
        desc.containsAtLeastOneNonProCity = false;
        desc.cities.resize(item->cities.count());

        const QVector<CityModelItem> cities = item->cities;
        for (const CityModelItem &city : cities) {
            if (!city.bShowPremiumStarOnly) {
                desc.containsAtLeastOneNonProCity = true;
                break;
            }
        }
        const auto connectionSpeed = PingTime(item->calcAveragePing()).toConnectionSpeed();
        desc.isEnabled = connectionSpeed != 0;

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

void LocationsTrayMenuNative::onSessionStatusChanged(bool bFreeSessionStatus)
{
    if (bIsFreeSession_ != bFreeSessionStatus) {
        bIsFreeSession_ = bFreeSessionStatus;
        rebuildMenu();
    }
}

void LocationsTrayMenuNative::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    auto it = locationsMap_.find(id);
    if (it != locationsMap_.end()) {
        LocationDesc &desc = locationsDesc_[it.value()];
        const bool isEnabled = timeMs.toConnectionSpeed() != 0;
        if (desc.isEnabled != isEnabled) {
            desc.isEnabled = isEnabled;
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
        if (!desc.containsAtLeastOneNonProCity && bIsFreeSession_)
            itemName += " (Pro)";

        IndependentPixmap *flag = ImageResourcesSvg::instance().getScaledFlag(
            desc.countryCode, 20 * G_SCALE, 10 * G_SCALE);

        if (desc.containsAtLeastOneNonProCity || !bIsFreeSession_) {
            desc.menu = addMenu(itemName);
            desc.menu->setEnabled(desc.isEnabled);
            if (flag)
                desc.menu->setIcon(flag->getIcon());
            connect(desc.menu, SIGNAL(triggered(QAction*)),
                    SLOT(onSubmenuActionTriggered(QAction*)));
        } else {
            QAction *action = addAction(itemName);
            action->setEnabled(false);
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
