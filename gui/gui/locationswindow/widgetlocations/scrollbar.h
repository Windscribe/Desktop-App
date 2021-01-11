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

protected:
    void wheelEvent(QWheelEvent *event) override;

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

};

#endif // SCROLLBAR_H
