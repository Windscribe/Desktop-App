#ifndef WIDGETCITIES_H
#define WIDGETCITIES_H

#include <QElapsedTimer>
#include "customscrollbar.h"
#include <QGraphicsScene>
#include <QTimer>
#include <QVBoxLayout>
#include <QAbstractScrollArea>
#include <QEasingCurve>
#include "icityitem.h"
#include "../backend/locationsmodel/basiccitiesmodel.h"
#include "../backend/locationsmodel/favoritelocationsstorage.h"
#include "../backend/types/types.h"
#include "iwidgetlocationsinfo.h"
#include "backgroundpixmapanimation.h"
#include "tooltips/tooltiptypes.h"

class FormConnect;

namespace GuiLocations {

class CursorUpdateHelper;

class WidgetCities : public QAbstractScrollArea, public IWidgetLocationsInfo
{
    Q_OBJECT

public:
    explicit WidgetCities(QWidget *parent);
    virtual ~WidgetCities() override;

    bool cursorInViewport() override;
    bool hasSelection() override;
    void centerCursorOnSelectedItem() override;

    void setModel(BasicCitiesModel *citiesModel);
    void setCurrentSelected(LocationID id);
    void setFirstSelected() override;
    void setExpanded(bool bExpanded);
    void startAnimationWithPixmap(const QPixmap &pixmap);

    void setShowLatencyInMs(bool showLatencyInMs);
    virtual bool isShowLatencyInMs() override;
    virtual bool isFreeSessionStatus() override;

    virtual int getWidth() override;
    virtual int getScrollBarWidth() override;

    void setCountAvailableItemSlots(int cnt);
    int getCountAvailableItemSlots();
    virtual QSize sizeHint() const override;

    void onKeyPressEvent(QKeyEvent *event);

    bool eventFilter(QObject *object, QEvent *event) override;

    void handleKeyEvent(QKeyEvent *event) override;

    QRect globalLocationsListViewportRect();

    int countVisibleItems() override;

    enum RibbonType { RIBBON_TYPE_NONE, RIBBON_TYPE_STATIC_IP, RIBBON_TYPE_CONFIG };
    void setRibbonInList(RibbonType ribbonType);

    void setSize(int width, int height);
    void updateScaling();

    void setDeviceName(const QString &deviceName);

    void setEmptyListDisplayIcon(QString emptyListDisplayIcon);
    void setEmptyListDisplayText(QString emptyListDisplayText);

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
    void heightChanged(int oldHeight, int newHeight);
    void addStaticIpURLClicked();
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId type);

private slots:
    void onItemsUpdated(QVector<CityModelItem*> items);
    void onConnectionSpeedChanged(LocationID id, PingTime timeMs);
    void onIsFavoriteChanged(LocationID id, bool isFavorite);
    void onFreeSessionStatusChanged(bool isFreeSessionStatus);

    void onTopScrollBarValueChanged(int value);

private:
    int width_;
    int height_;
    int topInd_;
    int topOffs_;
    int indSelected_;
    int topOffsSelected_;
    int indPressed_;
    bool indFavouritePressed_;

    int countOfAvailableItemSlots_;

    QVector<ICityItem *> items_;
    bool bIsFreeSession_;

    int startAnimationTop_;
    int endAnimationTop_;
    double scrollAnimationDuration_;
    QElapsedTimer scrollAnimationElapsedTimer_;
    bool isScrollAnimationNow_;

    double currentScale_;

    QEasingCurve easingCurve_;

    std::unique_ptr<CursorUpdateHelper> cursorUpdateHelper_;

    // variables for tooltip
    QPoint prevCursorPos_;
    bool bMouseInViewport_;

    bool bExpanded_;
    bool bShowLatencyInMs_;

    bool bTapGestureStarted_;

    CustomScrollBar *scrollBarHidden_;
    QScrollBar *scrollBarOnTop_;
    BasicCitiesModel *citiesModel_;

    QList<LocationID> currentVisibleItems_;

    BackgroundPixmapAnimation backgroundPixmapAnimation_;

    RibbonType ribbonType_;

    int getItemHeight() const;
    int getTopOffset() const;
    void calcScrollPosition();
    void setupScrollBar();
    void setupScrollBarMaxValue();
    bool detectSelectedItem(const QPoint &cursorPos);
    bool detectItemClickOnFavoriteLocationIcon();
    void setCursorForSelected();
    LocationID detectLocationForTopInd(int topInd);
    int detectVisibleIndForCursorPos(const QPoint &pt);

    void handleMouseMoveForTooltip();
    void handleLeaveForTooltip();

    QPoint adjustToolTipPosition(const QPoint &globalPoint, const QSize &toolTipSize, bool isP2PToolTip);
    QPoint getSelectItemCaption1TextCenter();

    void clearItems();
    double calcScrollingSpeed(double scrollItemsCount);
    bool isGlobalPointInViewport(const QPoint &pt);
    void scrollDownToSelected();
    void scrollUpToSelected();
    void handleTapClick(const QPoint &cursorPos);
    void emitSelectedIfNeed();

    int viewportPosYOfIndex(int index, bool centerY);

    void updateSelectionCursorAndToolTipByCursorPos();
    void updateListDisplay(QVector<CityModelItem*> items);

    QString deviceName_;
    QString emptyListDisplayIcon_;
    QString emptyListDisplayText_;
};

} // namespace GuiLocations

#endif // WIDGETCITIES_H
