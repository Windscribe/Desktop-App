#ifndef LOCATIONSTRAYMENUWIDGET_H
#define LOCATIONSTRAYMENUWIDGET_H

#include <QHash>
#include <QMenu>
#include <QWidget>
#include <QListWidget>
#include "locationstraymenutypes.h"
#include "locationstraymenuitemdelegate.h"
#include "locationstraymenubutton.h"
#include "types/locationid.h"
#include "types/pingtime.h"

class LocationsTrayMenuWidget : public QWidget
{
    Q_OBJECT
public:
    class LocationsTrayMenuWidgetSubmenu : public QMenu
    {
    public:
        explicit LocationsTrayMenuWidgetSubmenu(LocationsTrayMenuWidget *host)
            : QMenu(host), host_(host) {}

        void mouseMoveEvent(QMouseEvent *event) override
        {
            QMenu::mouseMoveEvent(event);
            if (!geometry().contains(QCursor::pos()))
                host_->handleMouseMove();
        }
        void wheelEvent(QWheelEvent *event) override
        {
            QMenu::wheelEvent(event);
            if (!geometry().contains(QCursor::pos()))
                host_->handleMouseWheel();
        }
    private:
        LocationsTrayMenuWidget *host_;
    };

    explicit LocationsTrayMenuWidget(LocationsTrayMenuType type, QWidget *parent = nullptr);
    ~LocationsTrayMenuWidget();

    virtual bool eventFilter(QObject *watched, QEvent *event);
    void handleMouseMove();
    void handleMouseWheel();

    void setLocationsModel(QAbstractItemModel *locationsModel);
    void setFontForItems(const QFont &font);

    static constexpr int USER_ROLE_FLAGS = Qt::UserRole + 1;
    static constexpr int USER_ROLE_TITLE = Qt::UserRole + 2;
    static constexpr int USER_ROLE_ORIGINAL_NAME = Qt::UserRole + 3;
    static constexpr int USER_ROLE_COUNTRY_CODE = Qt::UserRole + 4;
    static constexpr int USER_ROLE_CITY_INFO = Qt::UserRole + 5;

    static constexpr int ITEM_FLAG_IS_ENABLED = 1 << 0;
    static constexpr int ITEM_FLAG_IS_VALID = 1 << 1;
    static constexpr int ITEM_FLAG_IS_PREMIUM_ONLY = 1 << 2;
    static constexpr int ITEM_FLAG_HAS_COUNTRY = 1 << 3;
    static constexpr int ITEM_FLAG_HAS_SUBMENU = 1 << 4;

signals:
    void locationSelected(int type, QString locationTitle, int cityIndex);

private slots:
    void onListWidgetItemTriggered(QListWidgetItem *item);
    void onSubmenuActionTriggered(QAction *action);
    void updateTableViewSelection();
    void onScrollUpClick();
    void onScrollDownClick();

    /*void onItemsUpdated(QVector<LocationModelItem*> items);
    void onFavoritesUpdated(QVector<CityModelItem*> items);
    void onStaticIpsUpdated(QVector<CityModelItem*> items);
    void onCustomConfigsUpdated(QVector<CityModelItem*> items);*/
    void onSessionStatusChanged(bool bFreeSessionStatus);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);

private:
    LocationsTrayMenuType locationType_;
    bool bIsFreeSession_;
    QHash<LocationID, QListWidgetItem *> map_;
    QHash<const QListWidgetItem *, LocationsTrayMenuWidgetSubmenu *> menuMap_;
    LocationsTrayMenuItemDelegate *locationsTrayMenuItemDelegate_;
    QListWidget *listWidget_;
    LocationsTrayMenuWidgetSubmenu *currentSubmenu_;
    LocationsTrayMenuButton *upButton_;
    LocationsTrayMenuButton *downButton_;
    int visibleItemsCount_;

    void clearItems();
    void recalcSize();
    void updateShortenedTexts();
    void updateSubmenuForSelection();
    void updateButtonsState();
    void updateBackground_mac();
};

#endif // LOCATIONSTRAYMENUWIDGET_H
