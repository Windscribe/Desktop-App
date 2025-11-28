#pragma once

#include <QScrollArea>
#include <QAbstractItemModel>

#include "expandableitemswidget.h"
#include "countryitemdelegate.h"
#include "cityitemdelegate.h"
#include "commonwidgets/scrollbar.h"
#include "types/locationid.h"

namespace gui_locations {

class LocationsView : public QScrollArea
{
    Q_OBJECT

public:
    explicit LocationsView(QWidget *parent, QAbstractItemModel *model);
    ~LocationsView() override;

    void setModel(QAbstractItemModel *model);
    void setShowLocationLoad(bool isShowLocationLoad);
    void setShowCountryFlagForCity(bool isShowCountryFlagForCity);
    void updateScaling();
    bool isEmptyList() const { return widget_->isEmptyList(); }

    void expandAll() { widget_->expandAll(); }
    void collapseAll() { widget_->collapseAll(); }
    void scrollToTop();
    void updateSelected();

    int count() const;

    bool handleKeyPressEvent(QKeyEvent *event);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void selected(const LocationID &lid);
    void clickedOnPremiumStarCity();
    void emptyListStateChanged(bool isEmptyList);

private slots:
    void onScrollBarActionTriggered(int action);
    void onExpandingAnimationStarted(int top, int height);

private:
    static constexpr int kScrollBarWidth = 11;

    ExpandableItemsWidget *widget_;
    CountryItemDelegate *countryItemDelegate_;
    CityItemDelegate *cityItemDelegate_;
    CommonWidgets::ScrollBar *scrollBar_;

    void ensureVisible(int top, int bottom);
    bool isCursorInList();
    void moveCursorToItem(int top);
};

} // namespace gui_locations

