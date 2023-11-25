#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "loginbutton.h"

namespace LoginWindow {

class LoggingInWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit LoggingInWindowItem(QGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void startAnimation();
    void stopAnimation();

    void setMessage(const QString &msg);
    void setAdditionalMessage(const QString &msg);

private slots:
    void onSpinnerRotationChanged(const QVariant &value);

private:
    int curSpinnerRotation_;
    QVariantAnimation spinnerAnimation_;

    bool animationActive_;

    int targetRotation_;

    QString message_;
    QString additionalMessage_;

};

}
