#include "fontdescr.h"

FontDescr::FontDescr(int size, bool isBold, int stretch, qreal letterSpacing):
    size_(size),
    isBold_(isBold),
    stretch_(stretch),
    letterSpacing_(letterSpacing)
{

}

int FontDescr::size() const
{
    return size_;
}

bool FontDescr::isBold() const
{
    return isBold_;
}

int FontDescr::stretch() const
{
    return stretch_;
}

qreal FontDescr::letterSpacing() const
{
    return letterSpacing_;
}
