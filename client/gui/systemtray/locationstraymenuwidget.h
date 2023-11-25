#pragma once

#include <QMenu>
#include <QWidget>
#include <QListView>
#include "locationstraymenuitemdelegate.h"
#include "locationstraymenubutton.h"
#include "types/locationid.h"

class LocationsTrayMenuWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuWidget(QWidget *parent, QAbstractItemModel *model, const QFont &font, const QRect &trayIconGeometry);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void locationSelected(const LocationID &lid);

private slots:
    void onListViewClicked(const QModelIndex &index);
    void onSubmenuActionTriggered(QAction *action);
    void updateTableViewSelection();
    void onScrollUpClick();
    void onScrollDownClick();

    void onModelChanged();

private:
    class LocationsTrayMenuWidgetSubmenu;

    LocationsTrayMenuItemDelegate *locationsTrayMenuItemDelegate_;
    QListView *listView_;
    LocationsTrayMenuWidgetSubmenu *currentSubmenu_;
    QPersistentModelIndex currentIndexForSubmenu_;
    LocationsTrayMenuButton *upButton_;
    LocationsTrayMenuButton *downButton_;
    int visibleItemsCount_;

    static constexpr int LOWEST_LDPI = 96;
    double scale_;
    QScreen *screen_;

    void handleMouseMove();
    void handleMouseWheel();
    void recalcSize();
    void updateSubmenuForSelection();
    void updateButtonsState();
    void updateBackground_mac();
};
