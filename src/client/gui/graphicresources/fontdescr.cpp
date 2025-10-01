#include "fontdescr.h"

FontDescr::FontDescr(int size, int weight, int stretch, qreal letterSpacing):
    size_(size),
    weight_(weight),
    stretch_(stretch),
    letterSpacing_(letterSpacing)
{

}

int FontDescr::size() const
{
    return size_;
}

int FontDescr::weight() const
{
    return weight_;
}

int FontDescr::stretch() const
{
    return stretch_;
}

qreal FontDescr::letterSpacing() const
{
    return letterSpacing_;
}
