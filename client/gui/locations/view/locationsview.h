#pragma once

#include <QScrollArea>
#include <QAbstractItemModel>

#include "expandableitemswidget.h"
#include "countryitemdelegate.h"
#include "cityitemdelegate.h"
#include "scrollbar.h"
#include "types/locationid.h"

namespace gui_locations {

class LocationsView : public QScrollArea
{
    Q_OBJECT

public:
    explicit LocationsView(QWidget *parent, QAbstractItemModel *model);
    ~LocationsView() override;

    void setModel(QAbstractItemModel *model);
    void setShowLatencyInMs(bool isShowLatencyInMs);
    void setShowLocationLoad(bool isShowLocationLoad);
    void updateScaling();

    bool eventFilter(QObject *object, QEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;


signals:
     void selected(const LocationID &lid);

private slots:
    void onScrollBarActionTriggered(int action);

    void onExpandingAnimationStarted(int top, int height);
    void onExpandingAnimationProgress(qreal progress);

private:
    static constexpr int kScrollBarWidth = 8;

    ExpandableItemsWidget *widget_;
    CountryItemDelegate *countryItemDelegate_;  // todo move outside class
    CityItemDelegate *cityItemDelegate_;        // todo move outside class
    ScrollBar *scrollBar_;
    int lastScrollTargetPos_;

    struct {
        int top = 0;
        int height = 0;
        int topItemInvisiblePart = 0;
    } expandingAnimationParams_;

    // when we start the scroll animation we have to forbid the expanding animation
    // so that we don't have animation collisions
    bool isExpandingAnimationForbidden_;

    void adjustPosBetweenMinAndMax(int &pos);
    void ensureVisible(int top, int bottom);
};

} // namespace gui_locations

