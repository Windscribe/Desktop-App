#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "commongraphics/scalablegraphicsobject.h"
#include "captchaitem.h"

namespace LoginWindow {

class LoggingInWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit LoggingInWindowItem(QGraphicsObject *parent = nullptr);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setClickable(bool enabled);
    void updateScaling() override;

    void startAnimation();
    void stopAnimation();

    void setMessage(const QString &msg);
    void setAdditionalMessage(const QString &msg);

    void showCaptcha(const QString &background, const QString &slider, int top);

signals:
    void captchaResolved(const QString &captchaSolution, const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY);

private slots:
    void onSpinnerRotationChanged(const QVariant &value);
    void onCaptchaResolved(const QString &captchaSolution, const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY);

private:
    int curSpinnerRotation_;
    QVariantAnimation spinnerAnimation_;

    bool animationActive_;

    int targetRotation_;

    QString message_;
    QString additionalMessage_;

    CaptchaItem *captchaItem_ = nullptr;

    void updatePositions();
};

}
