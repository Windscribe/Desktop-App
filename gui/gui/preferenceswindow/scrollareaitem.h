#ifndef SCROLLAREAITEM_H
#define SCROLLAREAITEM_H

#include "commongraphics/scalablegraphicsobject.h"
#include <QGraphicsSceneWheelEvent>
#include "commongraphics/verticalscrollbar.h"
#include "preferenceswindow/basepage.h"

namespace PreferencesWindow {

class ScrollAreaItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ScrollAreaItem(ScalableGraphicsObject *parent, int height);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setItem(BasePage *item);

    void setHeight(int height);

    void setScrollBarVisibility(bool on);

    void hideOpenPopups();
    void updateScaling() override;

private slots:
    void onScrollBarMoved(double posPercentY);
    void onItemPosYChanged(const QVariant &value);

    void onPageHeightChange(int newHeight);
    void onPageScrollToPosition(int itemPos);
    void onPageScrollToRect(QRect r);

    void onScrollBarOpacityChanged(const QVariant &value);

protected:
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;

private:
    BasePage *curItem_;
    VerticalScrollBar *scrollBar_;

    int height_;

    static constexpr int WIDTH = 282;
    static constexpr int SCROLL_BAR_GAP = 1;

    QVariantAnimation itemPosAnimation_;

    double curScrollBarOpacity_;
    QVariantAnimation scrollBarOpacityAnimation_;

    void updateScrollBarByHeight();
    void updateScrollBarByItem();
    void setItemPosY(int newPosY);
    double screenFraction();

    bool scrollBarVisible_ = false;

};

} // namespace PreferencesWindow

#endif // SCROLLAREAITEM_H
