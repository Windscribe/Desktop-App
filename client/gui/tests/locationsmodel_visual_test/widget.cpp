#include "widget.h"
#include "ui_widget.h"
#include <QClipboard>
#include <QFile>
#include "utils/utils.h"
#include "locations/model/proxymodels/cities_proxymodel.h"
#include "locations/model/proxymodels/sortedcities_proxymodel.h"
#include "locations/model/proxymodels/staticips_proxymodel.h"
#include "locations/model/proxymodels/customconfigs_proxymodel.h"
#include <QtTest/QAbstractItemModelTester>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    {
        QFile file(":data/tests/locationsmodel/original.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testOriginal_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/deleted_locations.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testDeletedLocations_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/deleted_cities.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testDeletedCities_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/changed_locations_captions.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testChangedLocationsCaptions_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/changed_cities_captions.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testChangedCitiesCaptions_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/changed_locations_order.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testChangedLocationsOrder_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/changed_cities_order.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testChangedCitiesOrder_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/custom_config.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        customConfigLocation_ = types::Location::loadLocationFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/custom_config2.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        customConfigLocation2_ = types::Location::loadLocationFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/simple1.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testSimple1_ = types::Location::loadLocationsFromJson(arr);
    }
    {
        QFile file(":data/tests/locationsmodel/simple2.json");
        file.open(QIODevice::ReadOnly);
        QByteArray arr = file.readAll();
        testSimple2_ = types::Location::loadLocationsFromJson(arr);
    }


    locationsModel_ = new gui_locations::LocationsModel(this);
    new QAbstractItemModelTester(locationsModel_, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
    ui->treeViewLocations->setModel(locationsModel_);

    sortedLocationsModel_ = new gui_locations::SortedLocationsProxyModel(this);
    sortedLocationsModel_->setSourceModel(locationsModel_);
    sortedLocationsModel_->sort(0);
    ui->treeViewSortedLocations->setModel(sortedLocationsModel_);

    gui_locations::CitiesProxyModel *citiesModel = new gui_locations::CitiesProxyModel(this);
    new QAbstractItemModelTester(citiesModel, QAbstractItemModelTester::FailureReportingMode::Fatal, this);
    citiesModel->setSourceModel(locationsModel_);

    gui_locations::SortedCitiesProxyModel *sortedCitiesModel = new gui_locations::SortedCitiesProxyModel(this);
    sortedCitiesModel->setSourceModel(citiesModel);
    sortedCitiesModel->sort(0);

    ui->listViewFavorite->setModel(sortedCitiesModel);


    gui_locations::StaticIpsProxyModel *staticIpsModel = new gui_locations::StaticIpsProxyModel(this);
    staticIpsModel->setSourceModel(sortedCitiesModel);
    ui->listViewStaticIps->setModel(staticIpsModel);

    gui_locations::CustomConfgisProxyModel *customConfigModel = new gui_locations::CustomConfgisProxyModel(this);
    customConfigModel->setSourceModel(sortedCitiesModel);
    ui->listViewCustomConfig->setModel(customConfigModel);

    locationsView_ = new gui_locations::LocationsView(this, sortedLocationsModel_);
    locationsView_->setFixedSize(508, 450);
    locationsView_->updateScaling();
    ui->verticalLayout_6->addWidget(locationsView_);

    locationsModel_->updateLocations(LocationID(), testOriginal_);
    locationsModel_->updateCustomConfigLocation(customConfigLocation_);
    currentLocations_ = testOriginal_;

    connect(ui->lineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        sortedLocationsModel_->setFilter(text);
    });


    connect(ui->btnOriginal, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testOriginal_);
    });

    connect(ui->btnDeletedLocations, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testDeletedLocations_);
    });

    connect(ui->btnDeletedCities, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testDeletedCities_);
    });

    connect(ui->btnChangedLocations, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testChangedLocationsCaptions_);
    });

    connect(ui->btnChangedCities, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testChangedCitiesCaptions_);
    });

    connect(ui->btnChangedLocationsOrder, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testChangedLocationsOrder_);
    });

    connect(ui->btnChangedCitiesOrder, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testChangedCitiesOrder_);
    });

    connect(ui->btnClipboard, &QPushButton::clicked,
            [=]()
    {
        /*QClipboard *clipboard = QGuiApplication::clipboard();
        QVector<types::Location> locations = types::Location::loadLocationsFromJson(clipboard->text().toUtf8());
        updateLocations(LocationID(), locations);*/
    });

    connect(ui->btnClear, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), QVector<types::Location>());
    });

    connect(ui->btnSimple1, &QPushButton::clicked,
            [=]()
    {
        updateLocations(LocationID(), testSimple2_);
    });

    connect(ui->btnRandom, &QPushButton::clicked,
            [=]()
    {
        QVector<types::Location> randomLocations = generateRandomLocations();
        updateLocations(LocationID(), randomLocations);
    });

    connect(ui->btnCustomConfig1, &QPushButton::clicked,
            [=]()
    {
        locationsModel_->updateCustomConfigLocation(customConfigLocation_);
    });

    connect(ui->btnCustomConfig2, &QPushButton::clicked,
            [=]()
    {
        locationsModel_->updateCustomConfigLocation(customConfigLocation2_);
    });

    connect(ui->btnClearCustomConfig, &QPushButton::clicked,
            [=]()
    {
        locationsModel_->updateCustomConfigLocation(types::Location());
    });

    connect(ui->btnBestLocation, &QPushButton::clicked,
            [=]()
    {
        if (currentLocations_.size() > 0)
        {
            int indLocation;
            while (true) {
                indLocation = Utils::generateIntegerRandom(0, currentLocations_.size() - 1);
                if (!currentLocations_[indLocation].id.isStaticIpsLocation()) {
                    break;
                }
            }
            int indCity = Utils::generateIntegerRandom(0, currentLocations_[indLocation].cities.size() - 1);
            locationsModel_->updateBestLocation(currentLocations_[indLocation].cities[indCity].id.apiLocationToBestLocation());
        }
    });

    connect(ui->cbSort, &QComboBox::currentIndexChanged,
            [=](int index)
    {
        sortedLocationsModel_->setLocationOrder((ORDER_LOCATION_TYPE)index);
        sortedCitiesModel->setLocationOrder((ORDER_LOCATION_TYPE)index);
    });

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this] {
        int i = Utils::generateIntegerRandom(0, 6);
        if (i == 0) updateLocations(LocationID(), testOriginal_);
        else if (i == 1) updateLocations(LocationID(), testDeletedLocations_);
        else if (i == 2) updateLocations(LocationID(), testDeletedCities_);
        else if (i == 3) updateLocations(LocationID(), testChangedLocationsCaptions_);
        else if (i == 4) updateLocations(LocationID(), testChangedCitiesCaptions_);
        else if (i == 5) updateLocations(LocationID(), testChangedLocationsOrder_);
        else if (i == 6) updateLocations(LocationID(), testChangedCitiesOrder_);

    });
    //timer->start(100);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::updateLocations(const LocationID &bestLocation, const QVector<types::Location> &newLocations)
{
    locationsModel_->updateLocations(bestLocation, newLocations);
    currentLocations_ = newLocations;
}

QVector<types::Location> Widget::generateRandomLocations()
{
    QVector<types::Location> randomLocations;
    for (int i = 0; i < 100; ++i)
    {
        if (Utils::generateIntegerRandom(0, 2) != 1)
        {
            types::Location location;
            location.id = LocationID::createTopApiLocationId(i);
            location.countryCode = "CA";
            location.isNoP2P = false;
            location.isPremiumOnly = false;
            location.name = "Location" + QString::number(i);

            int citiesCount = Utils::generateIntegerRandom(0, 10);
            for (int c = 0; c < citiesCount; ++c)
            {
                types::City city;
                city.city = location.name + " City" + QString::number(c);
                city.nick = location.name + " Nick" + QString::number(c);
                city.id = LocationID::createApiLocationId(i, city.city, city.nick);
                city.pingTimeMs = Utils::generateIntegerRandom(0, 1900);
                city.isPro = (Utils::generateIntegerRandom(0, 1) == 1);
                city.isDisabled = (Utils::generateIntegerRandom(0, 1) == 1);
                city.is10Gbps = (Utils::generateIntegerRandom(0, 5) == 1);
                city.health = Utils::generateIntegerRandom(0, 100);
                location.cities << city;
            }
            randomLocations << location;
        }
    }

    return randomLocations;
}



