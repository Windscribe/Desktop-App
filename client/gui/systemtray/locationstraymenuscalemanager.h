#ifndef LOCATIONSTRAYMENUSCALEMANAGER_H
#define LOCATIONSTRAYMENUSCALEMANAGER_H

#include <QPushButton>

class LocationsTrayMenuScaleManager
{
public:

    static LocationsTrayMenuScaleManager &instance()
    {
        static LocationsTrayMenuScaleManager s;
        return s;
    }

    void setTrayIconGeometry(const QRect &geometry);
    double scale() const;
    QScreen *screen();

private:
    LocationsTrayMenuScaleManager();

    static constexpr int LOWEST_LDPI = 96;
    double scale_;
    QScreen *screen_;
};

#endif // LOCATIONSTRAYMENUSCALEMANAGER_H
