#ifndef LOCATIONSTRAYMENUWIDGET_H
#define LOCATIONSTRAYMENUWIDGET_H

#include <QHash>
#include <QMenu>
#include <QWidget>
#include <QListWidget>
#include "locationstraymenuitemdelegate.h"
#include "locationstraymenubutton.h"
#include "../backend/locationsmodel/locationsmodel.h"

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

    explicit LocationsTrayMenuWidget(QWidget *parent = nullptr);
    ~LocationsTrayMenuWidget();

    virtual bool eventFilter(QObject *watched, QEvent *event);
    void handleMouseMove();
    void handleMouseWheel();

    void setLocationsModel(LocationsModel *locationsModel);
    void setFontForItems(const QFont &font);

    static constexpr int USER_ROLE_ENABLED = Qt::UserRole + 1;
    static constexpr int USER_ROLE_TITLE = Qt::UserRole + 2;
    static constexpr int USER_ROLE_ORIGINAL_NAME = Qt::UserRole + 3;
    static constexpr int USER_ROLE_IS_PREMIUM_ONLY = Qt::UserRole + 4;
    static constexpr int USER_ROLE_COUNTRY_CODE = Qt::UserRole + 5;
    static constexpr int USER_ROLE_CITY_INFO = Qt::UserRole + 6;

signals:
    void locationSelected(QString locationTitle, int cityIndex);

private slots:
    void onSubmenuActionTriggered(QAction *action);
    void updateTableViewSelection();
    void onScrollUpClick();
    void onScrollDownClick();

    void onItemsUpdated(QVector<LocationModelItem*> items);
    void onSessionStatusChanged(bool bFreeSessionStatus);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);

private:
    bool bIsFreeSession_;
    QHash<LocationID, QListWidgetItem *> map_;
    QHash<const QListWidgetItem *, LocationsTrayMenuWidgetSubmenu *> menuMap_;
    LocationsTrayMenuItemDelegate *locationsTrayMenuItemDelegate_;
    QListWidget *listWidget_;
    LocationsTrayMenuWidgetSubmenu *currentSubmenu_;
    LocationsTrayMenuButton *upButton_;
    LocationsTrayMenuButton *downButton_;
    int visibleItemsCount_;

    void updateSubmenuForSelection();
    void updateButtonsState();
    void updateBackground_mac();
};

#endif // LOCATIONSTRAYMENUWIDGET_H
