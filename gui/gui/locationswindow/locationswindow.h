#ifndef LOCATIONSWINDOW_H
#define LOCATIONSWINDOW_H

#include <QWidget>
#include "WidgetLocations/locationstab.h"

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

    void updateLocationsTabGeometry();
    void updateScaling();

public slots:
    void setLatencyDisplay(ProtoTypes::LatencyDisplayType l);
    void onLanguageChanged();

signals:
    void heightChanged();
    void selected(LocationID id);
    void switchFavorite(LocationID id, bool isFavorite);
    void addStaticIpClicked();
    void addCustomConfigClicked();
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId type);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    const int BOTTOM_AREA_HEIGHT = 20;
    const int MIN_VISIBLE_LOCATIONS = 3;
    const int MAX_VISIBLE_LOCATIONS = 12;

    GuiLocations::LocationsTab *locationsTab_;
    int locationsTabHeight_;
    bool bDragPressed_;
    QPoint dragPressPt_;
    int dragInitialVisibleItemsCount_;
    int dragInitialBtnDragCenter_;

    QRect getResizeMiddleRect();

    QColor footerColor_;
};

#endif // LOCATIONSWINDOW_H
