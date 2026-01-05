#pragma once

#include <QWidget>
#include <QVariantAnimation>
#include "widgetlocations/locationstab.h"

namespace gui_locations
{
    class LocationsModelManager;
}

class LocationsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsWindow(QWidget *parent, Preferences *preferences, gui_locations::LocationsModelManager *locationsModelManager);

    int tabAndFooterHeight() const;

    void setViewportHeight(int unscaledHeight, bool dragging = false);
    int getCountVisibleItems();

    void updateLocationsTabGeometry();
    void updateScaling();

    void setTab(LOCATION_TAB tab);

    void setDataRemaining(qint64 bytesUsed, qint64 bytesMax);

public slots:
    void setCustomConfigsPath(QString path);
    void setShowLocationLoad(bool showLocationLoad);
    void onSearchFilterChanged(const QString &filter);
    void onLocationsKeyPressed(QKeyEvent *event);
    void onAppSkinChanged(APP_SKIN s);

signals:
    void heightChanged();
    void selected(const LocationID &lid);
    void clickedOnPremiumStarCity();
    void addStaticIpClicked();
    void clearCustomConfigClicked();
    void addCustomConfigClicked();
    void upgradeBannerClicked();

protected:
    void paintEvent(QPaintEvent *event)        override;
    void resizeEvent(QResizeEvent *event)      override;
    void mouseMoveEvent(QMouseEvent *event)    override;
    void mousePressEvent(QMouseEvent *event)   override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onResizeIconOpacityChanged(const QVariant &value);

private:
    const int LOCATIONS_TAB_HEIGHT_INIT = 398; // 7 * 50 + TAB_HEADER_HEIGHT
    static constexpr int FOOTER_HEIGHT = 16;
    static constexpr int FOOTER_HEIGHT_FULL = FOOTER_HEIGHT + 2;
    static constexpr int MIN_VISIBLE_LOCATIONS = 4;
    static constexpr int MAX_VISIBLE_LOCATIONS = 12;

    GuiLocations::LocationsTab *locationsTab_;
    QWidget *borderOverlay_;

    int locationsTabHeightUnscaled_;
    bool bDragPressed_;
    QPoint dragPressPt_;
    int dragInitialVisibleItemsCount_;
    int dragInitialViewportHeight_;
    int dragInitialBtnDragCenter_;
    double resizeIconOpacity_;
    QVariantAnimation resizeIconOpacityAnimation_;

    QRect getResizeHandleClickableRect();
    void updateFooterOverlayGeo();
};
