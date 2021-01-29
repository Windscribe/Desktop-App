#ifndef SEARCHWIDGETLOCATIONS_H
#define SEARCHWIDGETLOCATIONS_H

#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QEasingCurve>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "../backend/locationsmodel/favoritelocationsstorage.h"
#include "../backend/types/types.h"
#include "iwidgetlocationsinfo.h"
#include "backgroundpixmapanimation.h"
#include "tooltips/tooltiptypes.h"
#include "widgetlocationslist.h"
#include "scrollbar.h"

// TODO: regression: scrollbar width, background colour, margin
// TODO: regression: scrollbar opacity on un/hover

namespace GuiLocations {

class WidgetLocations : public QScrollArea, public IWidgetLocationsInfo
{
    Q_OBJECT

public:
    explicit WidgetLocations(QWidget *parent);
    ~WidgetLocations() override;

    void setModel(BasicLocationsModel *locationsModel);
    void setFilterString(QString text);
    void updateScaling() override;

    bool hasSelection() override;
    LocationID selectedItemLocationId() override;
    void setFirstSelected() override;

    bool cursorInViewport() override;
    void centerCursorOnSelectedItem() override;
    void centerCursorOnItem(LocationID locId) override;

    int countViewportItems() override;
    void setCountViewportItems(int cnt) override;

    virtual bool isShowLatencyInMs()   override;
    void setShowLatencyInMs(bool showLatencyInMs)override;
    virtual bool isFreeSessionStatus() override;

    void startAnimationWithPixmap(const QPixmap &pixmap) override;

    bool eventFilter(QObject *object, QEvent *event) override;
    void handleKeyEvent(QKeyEvent *event) override;

protected:
    virtual void paintEvent(QPaintEvent *event)            override;
    virtual void scrollContentsBy(int dx, int dy)          override;
    virtual void mouseMoveEvent(QMouseEvent *event)        override;
    virtual void mousePressEvent(QMouseEvent *event)       override;
    virtual void mouseReleaseEvent(QMouseEvent *event)     override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void leaveEvent(QEvent *event)                 override;
    virtual void enterEvent(QEvent *event)                 override;
    virtual void resizeEvent(QResizeEvent *event)          override;

signals:
    void selected(LocationID id);
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
    void onLocationItemListWidgetRegionExpanding(ItemWidgetRegion *region, WidgetLocationsList::ExpandReason reason);

    void onScrollAnimationValueChanged(const QVariant &value);
    void onScrollBarHandleDragged(int valuePos);

private:
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
    bool kickPreventMouseSelectionTimer_;
    QElapsedTimer preventMouseSelectionTimer_;
    int animationScollTarget_;
    QVariantAnimation scrollAnimation_;

    void updateWidgetList(QVector<LocationModelItem *> items);

    // scrolling
    const QString scrollbarStyleSheet();
    void scrollToIndex(int index);
    void scrollDown(int itemCount);
    void animatedScrollDown(int itemCount);
    void animatedScrollUp(int itemCount);

    // viewport
    const LocationID topViewportSelectableLocationId();
    int viewportOffsetIndex();
    int accentItemViewportIndex();
    int viewportIndex(LocationID locationId);
    bool locationIdInViewport(LocationID location);
    bool isGlobalPointInViewport(const QPoint &pt);
    QRect globalLocationsListViewportRect();

    // helper
    int getScrollBarWidth();
    int getItemHeight() const;
    void handleTapClick(const QPoint &cursorPos);
    int closestPositionIncrement(int value);

};

} // namespace GuiLocations


#endif // SEARCHWIDGETLOCATIONS_H
