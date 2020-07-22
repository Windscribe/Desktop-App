#ifndef SCROLLAREAWIDGET_H
#define SCROLLAREAWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include "verticalscrollbarwidget.h"
#include "CommonWidgets/iscrollablewidget.h"

class ScrollAreaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScrollAreaWidget(int width, int height, QWidget *parent = nullptr);
    QSize sizeHint() const override;

    void setCentralWidget(IScrollableWidget *widget);
    void setWidth(int width);
    void setHeight(int height);

    void setScrollBarColor(QColor color);
    void moveWidgetPosY(int newY);

    void updateSizing();

    double screenFraction();

    void setLayoutConstraint(QLayout::SizeConstraint constraint);

    int discretePosY(int newY);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onScrollBarMoved(double posPercentY);
    void onItemPosYChanged(const QVariant &value);

    void onWidgetShift(int newPos);
    void onRecenterWidget();

private:
    int width_;
    int height_;
    const int SCROLL_BAR_GAP = 2;

    int lastWidgetPos_;
    IScrollableWidget *centralWidget_;
    VerticalScrollBarWidget *scrollBar_;

    QVariantAnimation itemPosAnimation_;

    QHBoxLayout *hLayout_;

    int lowestY();
    void updateScrollBarPos();
    void updateScrollBarHeightAndPos();

};

#endif // SCROLLAREAWIDGET_H
