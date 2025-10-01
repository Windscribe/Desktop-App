#pragma once

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QFont>
#include <QVariant>
#include "verticalscrollbarwidget.h"
#include "combomenuwidgetbutton.h"

namespace CommonWidgets {

class ComboMenuWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ComboMenuWidget(QWidget *parent = nullptr);

    QPixmap getCurrentPixmapShape();

    int topMargin();
    int itemCount();
    int visibleItems();
    int itemHeight();

    void addItem(QString text, const QVariant &item_data);
    void removeItem(int index);
    void clearItems();

    int indexOfLastToppableItem();
    int indexOfItemByName(QString text);

    QString textOfItem(int index);
    void navigateItemToTop(int index);
    void activateItem(int index);

    void updateScaling();
    void setMaxItemsShowing(int maxItemsShowing);

signals:
    void itemClicked(QString text, QVariant item_data);
    void hidden();
    void requestScalingUpdate();
    void sizeChanged(int width, int height);

public slots:
    void onButtonClick();
    void onButtonHoverEnter();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void onScrollBarMoved(double posPercentY);

private:
    //QFont font_;
    static constexpr int SPACER_SIZE = 7;
    static constexpr int ROUNDED_CORNER = 7;

    // background
    QRect roundedBackgroundRect_;

    const int TRACKPAD_DELTA_THRESHOLD = 25;
    int trackpadDeltaSum_;
    VerticalScrollBarWidget *scrollBar_;

    // list widget
    int menuListPosY_;
    QVBoxLayout *layout_;
    QWidget *menuListWidget_;
    QRect clippingRegion_;
    QList<ComboMenuWidgetButton *> items_;

    ComboMenuWidgetButton *itemByName(QString text);
    int indexByButton(ComboMenuWidgetButton *button);

    void moveListPos(int newY);
    int sideMargin();
    int largestButtonWidthUnscaled();
    int allButtonsHeight();
    double scrollBarHeightFraction();
    int restrictedHeight();

    static constexpr int SCROLL_BAR_WIDTH = 6;
    static constexpr int MINIMUM_COMBO_MENU_WIDTH = 60;
    static constexpr int SHADOW_SIZE = 4;
    static constexpr int STEP_SIZE = 38;
    static constexpr int DEFAULT_MAX_ITEMS_SHOWING = 5;
    int maxItemsShowing_ = DEFAULT_MAX_ITEMS_SHOWING;
    int maxViewportHeight_ = DEFAULT_MAX_ITEMS_SHOWING * STEP_SIZE;

    void updateScrollBarVisibility();
};

}
