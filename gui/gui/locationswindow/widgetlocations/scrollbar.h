#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <QScrollBar>
#include <QElapsedTimer>
#include <QTimer>

// TODO: bug - drag scroller down then wheel mouse (will scroll from pre-scroller selection)
// TODO: bug - drag scroller -> doesn't move in animated notches, causes jittery viewport scrolling
class ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit ScrollBar(QWidget *parent = nullptr);
    void forceSetValue(int value); // sets the value and updates the target - avoids wheeling bugs where target is outdated

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
//    void mousePressEvent(QMouseEvent *event) override;
//    void mouseReleaseEvent(QMouseEvent *event) override;
//    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onScrollAnimationValueChanged(const QVariant &value);
    void onTimerTick();

private:
    const double SCROLL_SPEED_FRACTION = 0.5;
    const int MINIMUM_DURATION = 200;
    int targetValue_;
    int startValue_;

    int animationDuration_;
    QElapsedTimer elapsedTimer_;
    QTimer timer_;
    int lastCursorPos_;

    bool pressed_;

    int stepIncrement() const;
};

#endif // SCROLLBAR_H
