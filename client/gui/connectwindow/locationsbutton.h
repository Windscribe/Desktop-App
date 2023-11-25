#pragma once

#include <QFont>
#include <QGraphicsObject>
#include <QPropertyAnimation>
#include "../backend/backend.h"
#include "commongraphics/clickablegraphicsobject.h"
#include "commongraphics/imageitem.h"

namespace ConnectWindow {

class LocationsButton : public ClickableGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(qreal arrowRotation READ arrowRotation WRITE setArrowRotation NOTIFY arrowRotationChanged)

public:
    explicit LocationsButton(ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    qreal arrowRotation() const { return arrowRotation_; }
    void setArrowRotation(qreal r);

    void onLocationsExpandStateChanged(bool isExpanded);

    void setTextColor(QColor color);

    void updateScaling() override;

signals:
    void arrowRotationChanged();

private slots:
    void onBackgroundOpacityChange(const QVariant &value);

private:
    qreal arrowRotation_;
    ImageItem *arrowItem_;
    QPropertyAnimation *rotationAnimation_;

    QVariantAnimation backgroundOpacityAnimation_;
    double backgroundDarkOpacity_;
    double backgroundLightOpacity_;
    QString backgroundDark_;
    QString backgroundLight_;

    QColor curTextColor_;
    bool isExpanded_;

    void updatePositions();
};

} //namespace ConnectWindow
