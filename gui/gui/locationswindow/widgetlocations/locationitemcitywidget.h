#ifndef LOCATIONITEMCITYWIDGET_H
#define LOCATIONITEMCITYWIDGET_H

#include <QAbstractButton>
#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"


namespace GuiLocations {

class LocationItemCityWidget : public QAbstractButton
{
    Q_OBJECT
public:
    explicit LocationItemCityWidget(CityModelItem cityModelItem, QWidget *parent = nullptr);
    ~LocationItemCityWidget();

    void setShowLatencyMs(bool showLatencyMs);
    void updateScaling();

    static const int HEIGHT = 50;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSharedPointer<QLabel> cityLabel_;
    QSharedPointer<QLabel> nickLabel_;
    CityModelItem cityModelItem_;
};

}

#endif // LOCATIONITEMCITYWIDGET_H

