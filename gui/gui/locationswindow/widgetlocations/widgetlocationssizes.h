#ifndef WIDGETLOCATIONSSIZES_H
#define WIDGETLOCATIONSSIZES_H

#include <QColor>


namespace GuiLocations {

class WidgetLocationsSizes
{
public:
    static WidgetLocationsSizes& instance()
    {
        static WidgetLocationsSizes i;
        return i;
    }


    int getItemHeight();
    int getTopOffset();
    int getScrollBarWidth();
    QColor getBackgroundColor();
    double getScrollingSpeedKef();

private:
    WidgetLocationsSizes();

    int itemHeight_;
    int topOffset_;
    int scrollBarWidth_;
    double scrollingSpeedKef_;
    QColor backgroundColor_;

};
} // namespace GuiLocations

#endif // WIDGETLOCATIONSSIZES_H
