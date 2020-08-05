#ifndef LOCATIONSTRAYMENUWIDGET_H
#define LOCATIONSTRAYMENUWIDGET_H

#include <QWidget>
#include <QListWidget>
#include "locationstraymenuitemdelegate.h"
#include "locationstraymenubutton.h"
#include "../backend/locationsmodel/locationsmodel.h"

class LocationsTrayMenuWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuWidget(QWidget *parent = nullptr);
    ~LocationsTrayMenuWidget();

    // void paintEvent(QPaintEvent *event);

    virtual bool eventFilter(QObject *watched, QEvent *event); // needed?

    void setLocationsModel(LocationsModel *locationsModel);
    void setFontForItems(const QFont &font);

signals:
    void locationSelected(int locationId);

private slots:
    void updateTableViewSelection();
    void onScrollUpClick();
    void onScrollDownClick();

    void onItemsUpdated(QVector<LocationModelItem*> items);
    void onSessionStatusChanged(bool bFreeSessionStatus);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);

private:
    bool bIsFreeSession_;
    QMap<int, QListWidgetItem *> map_;
    LocationsTrayMenuItemDelegate *locationsTrayMenuItemDelegate_;
    QListWidget *listWidget_;
    LocationsTrayMenuButton *upButton_;
    LocationsTrayMenuButton *downButton_;

    static constexpr int VISIBLE_ITEMS_COUNT = 20;
    static constexpr int USER_ROLE_ENABLED = Qt::UserRole + 1;
    static constexpr int USER_ROLE_ID = Qt::UserRole + 2;
    static constexpr int USER_ROLE_ORIGINAL_NAME = Qt::UserRole + 3;
    static constexpr int USER_ROLE_IS_PREMIUM_ONLY = Qt::UserRole + 4;

    void updateButtonsState();
    void updateBackground_mac();
};

#endif // LOCATIONSTRAYMENUWIDGET_H
