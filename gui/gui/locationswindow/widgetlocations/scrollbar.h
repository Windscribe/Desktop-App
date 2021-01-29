#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <QScrollBar>
#include <QElapsedTimer>
#include <QTimer>

class ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit ScrollBar(QWidget *parent = nullptr);
    void forceSetValue(int value); // sets the value and updates the target - avoids wheeling bugs where target is outdated

    bool dragging();

signals:
    void handleDragged(int valuePos);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

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
    int lastValue_;

    bool pressed_;

    double magicRatio() const;
    void animateScroll(int target, int animationSpeedFraction);
};

#endif // SCROLLBAR_H
