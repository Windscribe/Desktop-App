#pragma once

#include "commongraphics/scalablegraphicsobject.h"
#include <QGraphicsSceneWheelEvent>
#include "commonwidgets/scrollbar.h"
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
    BasePage *item();

    void setHeight(int height);
    int scrollPos();
    void setScrollPos(int pos);
    void setScrollOffset(int amt);

    void setScrollBarVisibility(bool on);

    bool hasItemWithFocus();
    void hideOpenPopups();
    void updateScaling() override;

    bool handleKeyPressEvent(QKeyEvent *event);

private slots:
    void onPageHeightChange(int newHeight);
    void onPageScrollToPosition(int itemPos);

    void onScrollBarValueChanged(int value);
    void onScrollBarOpacityChanged(const QVariant &value);

    void onScrollBarActionTriggered(int action);

protected:
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    BasePage *curItem_;
    CommonWidgets::ScrollBar *scrollBar_;
    QGraphicsProxyWidget *scrollBarItem_;

    int height_;
    int width_;

    static constexpr int kScrollBarWidth = 11;
    static constexpr int SCROLL_DISTANCE_FROM_RIGHT = 5;
    static constexpr int SCROLL_BAR_GAP = 1;

    double curScrollBarOpacity_;
    QVariantAnimation scrollBarOpacityAnimation_;

    void updateScrollBarByHeight();
    void setItemPosY(int newPosY);
    double screenFraction();

    bool scrollBarVisible_ = false;
};

} // namespace CommonGraphics
