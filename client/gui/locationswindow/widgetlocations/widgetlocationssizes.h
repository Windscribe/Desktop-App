#ifndef WIDGETLOCATIONSSIZES_H
#define WIDGETLOCATIONSSIZES_H


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
    double getScrollingSpeedKef();

private:
    WidgetLocationsSizes();

    int itemHeight_;
    int topOffset_;
    int scrollBarWidth_;
    double scrollingSpeedKef_;

};
} // namespace GuiLocations

#endif // WIDGETLOCATIONSSIZES_H
