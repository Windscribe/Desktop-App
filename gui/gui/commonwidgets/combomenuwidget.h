#ifndef SCROLLABLEMENUWIDGET_H
#define SCROLLABLEMENUWIDGET_H

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

    void addItem(QString text, const QVariant &data);
    void removeItem(int index);
    void clearItems();

    int indexOfLastToppableItem();
    int indexOfItemByName(QString text);

    QString textOfItem(int index);
    void navigateItemToTop(int index);
    void activateItem(int index);

    void updateScaling();

signals:
    void itemClicked(QString text, QVariant data);
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
    const int SPACER_SIZE = 7;

    // background
    QRect roundedBackgroundRect_;
    const int roundedCorner_;

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
    int largestButtonWidth();

    const int SHADOW_SIZE = 4;
    const int STEP_SIZE = 38;
    int MAX_ITEMS_SHOWING = 5;
    int maxViewportHeight_ = MAX_ITEMS_SHOWING * STEP_SIZE;

};

}

#endif // SCROLLABLEMENUWIDGET_H
