#ifndef LOCATIONSWINDOW_H
#define LOCATIONSWINDOW_H

#include <QWidget>
#include "widgetlocations/locationstab.h"
#include "widgetlocations/footer.h"

class LocationsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsWindow(QWidget *parent, LocationsModel *locationsModel);

    int tabAndFooterHeight() const;

    void setCountVisibleItemSlots(int cnt);
    int getCountVisibleItems();

    void setOnlyConfigTabVisible(bool onlyConfig);

    void handleKeyReleaseEvent(QKeyEvent *event);
    void handleKeyPressEvent(QKeyEvent *event);

    void updateLocationsTabGeometry();
    void updateScaling();

    void hideSearchTabWithoutAnimation();
    GuiLocations::LocationsTab::LocationTabEnum currentTab();

public slots:
    void setLatencyDisplay(ProtoTypes::LatencyDisplayType l);
    void setCustomConfigsPath(QString path);
    void onLanguageChanged();

signals:
    void heightChanged();
    void selected(LocationID id);
    void clickedOnPremiumStarCity();
    void switchFavorite(LocationID id, bool isFavorite);
    void addStaticIpClicked();
    void clearCustomConfigClicked();
    void addCustomConfigClicked();

protected:
    void paintEvent(QPaintEvent *event)        override;
    void resizeEvent(QResizeEvent *event)      override;
    void mouseMoveEvent(QMouseEvent *event)    override;
    void mousePressEvent(QMouseEvent *event)   override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    const int LOCATIONS_TAB_HEIGHT_INIT = 398; // 7 * 50 + TAB_HEADER_HEIGHT
    static constexpr int FOOTER_HEIGHT = 22; // 20 + COVER_LAST_ITEM_LINE
    static constexpr int MIN_VISIBLE_LOCATIONS = 3;
    static constexpr int MAX_VISIBLE_LOCATIONS = 12;

    GuiLocations::LocationsTab *locationsTab_;
    GuiLocations::Footer *footer_; // overlay needed to be done as child widget so we can paint over the other widget, locationsTab_

    int locationsTabHeight_;
    bool bDragPressed_;
    QPoint dragPressPt_;
    int dragInitialVisibleItemsCount_;
    int dragInitialBtnDragCenter_;

    QRect getResizeMiddleRect();

    QColor footerColor_;
};

#endif // LOCATIONSWINDOW_H
