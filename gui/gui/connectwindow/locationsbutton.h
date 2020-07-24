#ifndef LOCATIONSBUTTON_H
#define LOCATIONSBUTTON_H

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

    virtual QRectF boundingRect() const;
    QPainterPath shape() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    qreal arrowRotation() const { return arrowRotation_; }
    void setArrowRotation(qreal r);

    void onLocationsExpandStateChanged(bool isExpanded);

    void setTextColor(QColor color);

    virtual void updateScaling();

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

#endif // LOCATIONSBUTTON_H
