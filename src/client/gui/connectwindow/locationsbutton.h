#pragma once

#include <QFont>
#include <QGraphicsObject>
#include <QPropertyAnimation>
#include "backend/backend.h"
#include "commongraphics/clickablegraphicsobject.h"
#include "commongraphics/imageitem.h"
#include "locationsmenu.h"

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

    void setIsExternalConfigMode(bool isExternalConfigMode);

    bool handleKeyPressEvent(QKeyEvent *event);

signals:
    void arrowRotationChanged();
    void locationTabClicked(LOCATION_TAB tab);
    void searchFilterChanged(const QString &filter);
    void locationsKeyPressed(QKeyEvent *event);

private slots:
    void onOpacityValueChanged(const QVariant &value);
    void onHoverEnter();
    void onHoverLeave();
    void onLanguageChanged();

private:
    qreal arrowRotation_;
    ImageItem *arrowItem_;
    QPropertyAnimation *rotationAnimation_;
    QVariantAnimation opacityAnimation_;
    double curOpacity_;

    LocationsMenu *locationsMenu_;

    QString background_;

    QColor curTextColor_;
    bool isExpanded_;

    void updatePositions();
};

} //namespace ConnectWindow
