#pragma once

#include <QScrollArea>
#include <QAbstractItemModel>
#include "expandableitemswidget.h"
#include "countryitemdelegate.h"
#include "cityitemdelegate.h"
#include "scrollbar.h"

namespace gui_locations {

class LocationsView : public QScrollArea
{
    Q_OBJECT

public:
    explicit LocationsView(QWidget *parent);
    ~LocationsView() override;

    void setModel(QAbstractItemModel *model);
    void setCountViewportItems(int cnt);
    void setShowLatencyInMs(bool isShowLatencyInMs);
    void setShowLocationLoad(bool isShowLocationLoad);
    void updateScaling();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:

private slots:


private:
    const int SCROLL_BAR_WIDTH = 10;
    ExpandableItemsWidget *widget_;
    CountryItemDelegate *countryItemDelegate_;  // todo move outside class
    CityItemDelegate *cityItemDelegate_;        // todo move outside class
    ScrollBar *scrollBar_;


};

} // namespace gui_locations

