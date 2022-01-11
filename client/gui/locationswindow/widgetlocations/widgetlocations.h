#ifndef SEARCHWIDGETLOCATIONS_H
#define SEARCHWIDGETLOCATIONS_H

#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QEasingCurve>
#include "backend/locationsmodel/basiclocationsmodel.h"
#include "backend/locationsmodel/favoritelocationsstorage.h"
#include "iwidgetlocationsinfo.h"
#include "backgroundpixmapanimation.h"
#include "tooltips/tooltiptypes.h"
#include "widgetlocationslist.h"
#include "scrollbar.h"

namespace GuiLocations {

class WidgetLocations : public QScrollArea, public IWidgetLocationsInfo
{
    Q_OBJECT

public:
    explicit WidgetLocations(QWidget *parent, const QString name);
    ~WidgetLocations() override;

    void setModel(BasicLocationsModel *locationsModel);
    void setFilterString(QString text);
    void updateScaling() override;

    bool hasAccentItem() override;
    LocationID accentedItemLocationId() override;
    void accentFirstItem() override;
    void setMuteAccentChanges(bool mute) override;

    bool cursorInViewport() override;
    void centerCursorOnAccentedItem() override;
    void centerCursorOnItem(LocationID locId) override;

    int countViewportItems() override;
    void setCountViewportItems(int cnt) override;

    virtual bool isShowLatencyInMs()   override;
    void setShowLatencyInMs(bool showLatencyInMs)override;
    virtual bool isFreeSessionStatus() override;

    void startAnimationWithPixmap(const QPixmap &pixmap) override;

    bool eventFilter(QObject *object, QEvent *event) override;
    void handleKeyEvent(QKeyEvent *event) override;

    int gestureScrollingElapsedTime() override;

protected:
    virtual void paintEvent(QPaintEvent *event)            override;
    virtual void scrollContentsBy(int dx, int dy)          override;
    virtual void mousePressEvent(QMouseEvent *event)       override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
    void selected(LocationID id);
    void clickedOnPremiumStarCity();
    void switchFavorite(LocationID id, bool isFavorite);

private slots:
    void onItemsUpdated(QVector<LocationModelItem*> items);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);
    void onIsFavoriteChanged(LocationID id, bool isFavorite);
    void onFreeSessionStatusChanged(bool isFreeSessionStatus);

    void onLanguageChanged();

    void onLocationItemListWidgetHeightChanged(int listWidgetHeight);
    void onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *cityWidget, bool favorited);
    void onLocationItemListWidgetLocationIdSelected(LocationID id);
    void onLocationItemListWidgetRegionExpanding(ItemWidgetRegion *region);

    void onScrollAnimationValueChanged(const QVariant &value);
    void onScrollAnimationFinished();
    void onScrollAnimationForKeyPressValueChanged(const QVariant &value);
    void onScrollAnimationForKeyPressFinished();
    void onScrollBarHandleDragged(int valuePos);
    void onScrollBarStopScroll(bool lastScrollDirectionUp);

private:
    QString name_;
    WidgetLocationsList *widgetLocationsList_;
    ScrollBar *scrollBar_;

    QString filterString_;
    int countOfAvailableItemSlots_;
    bool bIsFreeSession_;
    bool bShowLatencyInMs_;
    bool bTapGestureStarted_;
    BasicLocationsModel *locationsModel_;
    BackgroundPixmapAnimation backgroundPixmapAnimation_;
    double currentScale_;

    int lastScrollPos_;
    const int PROGRAMMATIC_SCROLL_ANIMATION_DURATION = 300;
    QElapsedTimer preventMouseSelectionTimer_;
    int animationScollTarget_;
    QVariantAnimation scrollAnimation_;
    QVariantAnimation scrollAnimationForKeyPress_;

    QElapsedTimer gestureScrollingElapsedTimer_;

    void updateWidgetList(QVector<LocationModelItem *> items);

    // scrolling
    void scrollToIndex(int index);
    void scrollDown(int itemCount);
    void animatedScrollDown(int itemCount);
    void animatedScrollUp(int itemCount);
    void animatedScrollDownByKeyPress(int itemCount);
    void animatedScrollUpByKeyPress(int itemCount);
    void startAnimationScrollByPosition(int positionValue, QVariantAnimation &animation);
    const int GESTURE_SCROLL_ANIMATION_DURATION = 200;
    void gestureScrollAnimation(int value);
    void updateScrollBarWithView();

    // viewport
    const LocationID topViewportSelectableLocationId();
    int viewportOffsetIndex();
    int accentItemViewportIndex();
    int viewportIndex(LocationID locationId);
    bool locationIdInViewport(LocationID location);
    bool isGlobalPointInViewport(const QPoint &pt);
    QRect globalLocationsListViewportRect();
    int regionViewportIndex(ItemWidgetRegion *region);
    int regionOutOfViewBy(ItemWidgetRegion *region);

    // helper
    int getScrollBarWidth();
    int getItemHeight() const;
    int closestPositionIncrement(int value);
    int nextPositionIncrement(int value);
    int previousPositionIncrement(int value);

    bool heightChanging_;
    void regionExpandingAnimation(ItemWidgetRegion *region);


};

} // namespace GuiLocations


#endif // SEARCHWIDGETLOCATIONS_H
