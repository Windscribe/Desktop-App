#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPainter>
#include <QVariantAnimation>
#include "numberspixmap.h"
#include "numberitem.h"

class OctetItem : public QObject
{
    Q_OBJECT
public:
    explicit OctetItem(QObject *parent, NumbersPixmap *numbersPixmap);

    void setOctetNumber(int num, bool bWithAnimation);
    void draw(QPainter *painter, int x, int y);
    int width() const;

    void recalcSizes();

signals:
    void needUpdate();
    void widthChanged();

private slots:
    void onTimer();

private:
    void setNums(int input, int &n1, int &n2, int &n3);

private:
    NumbersPixmap *numbersPixmap_;
    NumberItem *numberItem_[3];
    int curNums_[3];
    int curWidth_;

    int oldWidth_;
    int newWidth_;

    QTimer *timer_;
    QElapsedTimer elapsed_;

    int calcWidth();

};
