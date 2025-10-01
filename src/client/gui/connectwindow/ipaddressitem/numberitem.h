#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPainter>
#include <QVariantAnimation>
#include "numberspixmap.h"

class NumberItem : public QObject
{
    Q_OBJECT
public:
    explicit NumberItem(QObject *parent, NumbersPixmap *numbersPixmap);

    void setNumber(int num);
    void startAnimation(int timeAcceleration);

    void draw(QPainter *painter, int x, int y);

    int width() const;

signals:
    void needUpdate();

private slots:
    void onTimer();

private:
    NumbersPixmap *numbersPixmap_;
    int curNum_;
    double offs_;
    double stopOffs_;
    double stopAccelerationOffs_;
    int prev_offs_;
    double curSpeed_;
    double prevSpeed_;
    int state_;

    QTimer *timer_;
    QElapsedTimer elapsed_;
    QElapsedTimer elapsedFrame_;

    static constexpr int CNT_SCROLLING_NUMBERS_ON_ACCELERATION = 6;
    int TIME_ACCELERATION;
    double acceleration_;
    double accelerationStopping_;

    int detectNumFromOffs(double offs);
    int distBetweenNumbers(int n1, int n2);
};
