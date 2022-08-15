#include <QTest>
#include "types/locationitem.h"
#include "locationitem_wrapper.h"

// tests for classes LocationsModel and CitiesModel
// CitiesModel depends on class LocationsModel data so it makes sense to test them together
class TestLocationItemWrapper : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testBasic();
    void testFindRemovedLocations();
    void testFindNewLocations();


private:
    QVector<gui::LocationItemWrapper> locationsOriginal_;
};


void TestLocationItemWrapper::init()
{
    QVector<types::LocationItem> l;
    QFile file(":data/tests/locationsmodel/original.json");
    file.open(QIODevice::ReadOnly);
    QVERIFY(file.isOpen());
    QByteArray arr = file.readAll();
    l = types::LocationItem::loadLocationsFromJson(arr);
    for (const auto &i : l)
    {
        locationsOriginal_ << gui::LocationItemWrapper(i);
    }
}

void TestLocationItemWrapper::cleanup()
{
    locationsOriginal_.clear();
}

void TestLocationItemWrapper::testBasic()
{

}

void TestLocationItemWrapper::testFindRemovedLocations()
{
    QVector<gui::LocationItemWrapper> locationsChanged = locationsOriginal_;
    locationsChanged.removeAt(9);
    locationsChanged.removeAt(8);
    locationsChanged.removeAt(2);

    QVector<int> removedInd = gui::LocationItemWrapper::findRemovedLocations(locationsOriginal_, locationsChanged);
    QVERIFY(removedInd.size() == 3);
    QVERIFY(std::find(removedInd.begin(),  removedInd.end(), 9) != removedInd.end());
    QVERIFY(std::find(removedInd.begin(),  removedInd.end(), 8) != removedInd.end());
    QVERIFY(std::find(removedInd.begin(),  removedInd.end(), 2) != removedInd.end());
}

void TestLocationItemWrapper::testFindNewLocations()
{
    QVector<gui::LocationItemWrapper> locationsChanged = locationsOriginal_;
    locationsOriginal_.removeAt(7);
    locationsOriginal_.removeAt(4);
    locationsOriginal_.removeAt(1);

    QVector<int> newInd = gui::LocationItemWrapper::findNewLocations(locationsOriginal_, locationsChanged);
    QVERIFY(newInd.size() == 3);
    QVERIFY(std::find(newInd.begin(),  newInd.end(), 1) != newInd.end());
    QVERIFY(std::find(newInd.begin(),  newInd.end(), 4) != newInd.end());
    QVERIFY(std::find(newInd.begin(),  newInd.end(), 7) != newInd.end());
}

QTEST_MAIN(TestLocationItemWrapper)
#include "locationitem_wrapper.test.moc"
