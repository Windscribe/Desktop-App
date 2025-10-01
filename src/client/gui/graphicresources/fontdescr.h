#pragma once

#include <QObject>

class FontDescr
{
public:
    FontDescr(int size, int weight, int stretch = 100, qreal letterSpacing = 0.0);

    int size() const;
    int weight() const;
    int stretch() const;
    qreal letterSpacing() const;

private:
    int size_;
    int weight_;
    int stretch_;
    qreal letterSpacing_;
};
