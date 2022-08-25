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
    void setShowLatencyInMs(bool isShowLatencyInMs);
    void setShowLocationLoad(bool isShowLocationLoad);
    void updateScaling();

    bool eventFilter(QObject *object, QEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;

signals:

private slots:
    void onScrollBarHandleDragged(int valuePos);
    void onScrollBarStopScroll(bool lastScrollDirectionUp);
    void onScrollAnimationValueChanged(const QVariant &value);
    void onScrollAnimationFinished();
    void onNotifyMustBeVisible(int topItemIndex, int bottomItemIndex);

private:
    const int SCROLL_BAR_WIDTH = 8;
    const int PROGRAMMATIC_SCROLL_ANIMATION_DURATION = 300;

    ExpandableItemsWidget *widget_;
    CountryItemDelegate *countryItemDelegate_;  // todo move outside class
    CityItemDelegate *cityItemDelegate_;        // todo move outside class
    ScrollBar *scrollBar_;
    int animationScollTarget_;
    int lastScrollPos_;
    QVariantAnimation scrollAnimation_;


    void startAnimationScrollByPosition(int positionValue, QVariantAnimation &animation);
    int nextPositionIncrement(int value);
    int previousPositionIncrement(int value);
    void updateScrollBarWithView();
};

} // namespace gui_locations

