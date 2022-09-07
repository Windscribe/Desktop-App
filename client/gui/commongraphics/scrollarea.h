#ifndef SCROLLAREA_H
#define SCROLLAREA_H

#include "commongraphics/scalablegraphicsobject.h"
#include <QGraphicsSceneWheelEvent>
#include "commongraphics/verticalscrollbar.h"
#include "basepage.h"

namespace CommonGraphics {

class ScrollArea : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ScrollArea(ScalableGraphicsObject *parent, int height, int width);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setItem(BasePage *item);

    void setHeight(int height);
    void setScrollPos(int amt);

    void setScrollBarVisibility(bool on);

    bool hasItemWithFocus();
    void hideOpenPopups();
    void updateScaling() override;

private slots:
    void onScrollBarMoved(double posPercentY);
    void onItemPosYChanged(const QVariant &value);

    void onPageHeightChange(int newHeight);
    void onPageScrollToPosition(int itemPos);
    void onPageScrollToRect(QRect r);

    void onScrollBarOpacityChanged(const QVariant &value);
    void onScrollBarHoverEnter();
    void onScrollBarHoverLeave();

protected:
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;

private:
    BasePage *curItem_;
    VerticalScrollBar *scrollBar_;

    int height_;
    int width_;

    static constexpr int SCROLL_DISTANCE_FROM_RIGHT = 2;
    static constexpr int SCROLL_BAR_GAP = 1;

    QVariantAnimation itemPosAnimation_;

    double curScrollBarOpacity_;
    QVariantAnimation scrollBarOpacityAnimation_;

    void updateScrollBarByHeight();
    void updateScrollBarByItem();
    void setItemPosY(int newPosY);
    double screenFraction();

    bool scrollBarVisible_ = false;

    const int TRACKPAD_DELTA_THRESHOLD = 120;
    int trackpadDeltaSum_;

    int scaledThreshold();
};

} // namespace CommonGraphics

#endif // SCROLLAREA_H
