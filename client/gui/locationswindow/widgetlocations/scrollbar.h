#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <QScrollBar>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantAnimation>

namespace GuiLocations {

class ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit ScrollBar(QWidget *parent = nullptr);
    void forceSetValue(int value); // sets the value and updates the target - avoids wheeling bugs where target is outdated

    bool dragging();
    void updateCustomStyleSheet();

signals:
    void handleDragged(int valuePos);
    void stopScroll(bool lastScrollUp);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onScrollAnimationValueChanged(const QVariant &value);
    void onScollTimerTick();
    void onOpacityAnimationValueChanged(const QVariant &value);
private:
    const double SCROLL_SPEED_FRACTION = 0.5;
    const int MINIMUM_DURATION = 200;
    int targetValue_;
    int startValue_;

    int animationDuration_;
    QElapsedTimer scrollElapsedTimer_;
    QTimer scrollTimer_;
    int lastCursorPos_;
    int lastValue_;
    bool lastScrollDirectionUp_;

    int trackpadDeltaSum_;
    int trackPadScrollDelta_;

    bool pressed_;

    double magicRatio() const;
    void animateScroll(int target, int animationSpeedFraction);

    const QString customStyleSheet();
    int customScrollBarWidth();
    int customPaddingWidth();

    double curOpacity_;
    QVariantAnimation opacityAnimation_;
};

} // namepspace
#endif // SCROLLBAR_H
