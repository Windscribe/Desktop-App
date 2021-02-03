#ifndef WIDGETCITIES_H
#define WIDGETCITIES_H

#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QEasingCurve>
#include "../backend/locationsmodel/basiccitiesmodel.h"
#include "../backend/locationsmodel/favoritelocationsstorage.h"
#include "../backend/types/types.h"
#include "commonwidgets/textbuttonwidget.h"
#include "iwidgetlocationsinfo.h"
#include "backgroundpixmapanimation.h"
#include "tooltips/tooltiptypes.h"
#include "widgetcitieslist.h"
#include "scrollbar.h"

namespace GuiLocations {

class WidgetCities : public QScrollArea, public IWidgetLocationsInfo
{
    Q_OBJECT
public:
    explicit WidgetCities(QWidget *parent, int visible_item_slots = 7);
    ~WidgetCities() override;

    void setModel(BasicCitiesModel *citiesModel);
    void updateScaling() override;

    bool hasSelection() override;
    LocationID selectedItemLocationId() override;
    void setFirstSelected() override;

    bool cursorInViewport() override;
    void centerCursorOnSelectedItem() override;
    void centerCursorOnItem(LocationID locId) override;

    int countViewportItems() override;
    void setCountViewportItems(int cnt) override;

    void setShowLatencyInMs(bool showLatencyInMs) override;
    virtual bool isShowLatencyInMs() override;
    virtual bool isFreeSessionStatus() override;

    void startAnimationWithPixmap(const QPixmap &pixmap) override;

    bool eventFilter(QObject *object, QEvent *event) override;

    void setEmptyListDisplayIcon(QString emptyListDisplayIcon);
    void setEmptyListDisplayText(QString emptyListDisplayText, int width = 0);
    void setEmptyListButtonText(QString text);

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
    void emptyListButtonClicked();

private slots:
    void onItemsUpdated(QVector<CityModelItem*> items);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);
    void onIsFavoriteChanged(LocationID id, bool isFavorite);
    void onFreeSessionStatusChanged(bool isFreeSessionStatus);

    void onLanguageChanged();

    void onLocationItemListWidgetHeightChanged(int listWidgetHeight);
    void onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *cityWidget, bool favorited);
    void onLocationItemListWidgetLocationIdSelected(LocationID id);

    void onScrollAnimationValueChanged(const QVariant &value);
    void onScrollBarHandleDragged(int valuePos);
private:
    WidgetCitiesList *widgetCitiesList_;
    ScrollBar *scrollBar_;
    BasicCitiesModel *citiesModel_;

    int countOfAvailableItemSlots_;
    bool bIsFreeSession_;
    bool bShowLatencyInMs_;
    bool bTapGestureStarted_;
    BackgroundPixmapAnimation backgroundPixmapAnimation_;
    double currentScale_;

    int lastScrollPos_;
    const int PROGRAMMATIC_SCROLL_ANIMATION_DURATION = 300;
    bool kickPreventMouseSelectionTimer_;
    QElapsedTimer preventMouseSelectionTimer_;
    int animationScollTarget_;
    QVariantAnimation scrollAnimation_;

    QString emptyListDisplayIcon_;
    QString emptyListDisplayText_;
    int emptyListDisplayTextWidth_;
    int emptyListDisplayTextHeight_;
    CommonWidgets::TextButtonWidget *emptyListButton_;

    void updateEmptyListButton();
    void updateWidgetList(QVector<CityModelItem*> items);

    // scrolling
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
    int getScrollBarWidth() ;
    int getItemHeight() const;
    void handleTapClick(const QPoint &cursorPos);
    int closestPositionIncrement(int value);

    int heightChanging_;
};

} // namespace GuiLocations

#endif // WIDGETCITIES_H
