#pragma once

#include <QObject>

class FontDescr
{
public:
    FontDescr(int size, bool isBold, int stretch = 100, qreal letterSpacing = 0.0);

    int size() const;
    bool isBold() const;
    int stretch() const;
    qreal letterSpacing() const;

private:
    int size_;
    bool isBold_;
    int stretch_;
    qreal letterSpacing_;
};
