#ifndef LOCATIONITEMCITYWIDGET_H
#define LOCATIONITEMCITYWIDGET_H

#include <QWidget>
#include <QLabel>
#include "citynode.h"

namespace GuiLocations {

class LocationItemCityWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationItemCityWidget(QSharedPointer<CityNode> cityNode, QWidget *parent = nullptr);
    explicit LocationItemCityWidget(QWidget *parent = nullptr);

    void setCity(QSharedPointer<CityNode> city);
    void setShowLatencyMs(bool showLatencyMs);
    void updateScaling();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const int HEIGHT = 50;
    QLabel textLabel_;
    QSharedPointer<CityNode> cityNode_;
};

}

#endif // LOCATIONITEMCITYWIDGET_H

