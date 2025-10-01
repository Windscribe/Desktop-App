#pragma once

#include <QGraphicsProxyWidget>
#include <QString>
#include <QTimer>

#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commonwidgets/custommenulineedit.h"
#include "types/enums.h"

namespace ConnectWindow {

class LocationsMenu : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit LocationsMenu(ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setIsExternalConfigMode(bool isExternalConfigMode);

    bool handleKeyPressEvent(QKeyEvent *event);
    void dismiss();

signals:
    void locationTabClicked(LOCATION_TAB tab);
    void searchFilterChanged(const QString &filter);
    void locationsKeyPressed(QKeyEvent *event);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private slots:
    void onAllLocationsClicked();
    void onConfiguredLocationsClicked();
    void onStaticIpsLocationsClicked();
    void onFavoriteLocationsClicked();
    void onSearchLocationsClicked();
    void onSearchCancelClicked();

    void onLanguageChanged();
    void onSearchLineEditTextChanged(const QString &text);
    void onSearchTypingDelayTimerTimeout();
    void onSearchAnimationValueChanged(const QVariant &value);
    void onSearchAnimationFinished();

private:
    IconButton *allLocations_;
    IconButton *configuredLocations_;
    IconButton *staticIpsLocations_;
    IconButton *favoriteLocations_;
    IconButton *searchLocations_;
    IconButton *searchCancel_;
    CommonWidgets::CustomMenuLineEdit *searchLineEdit_;

    QGraphicsProxyWidget *lineEditProxy_;

    QVariantAnimation searchAnimation_;
    int curSearchPos_;

    QTimer searchTypingDelayTimer_;

    QString filterText_;
    LOCATION_TAB curTab_;
    LOCATION_TAB prevTab_;

    bool isExternalConfigMode_;

    void updatePositions();
    void updateSelectedTab(IconButton *button);
    void showSearch();
    void hideSearch();
    bool handleLeftKey();
    bool handleRightKey();
};

} //namespace ConnectWindow
