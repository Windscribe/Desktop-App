#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "commongraphics/scalablegraphicsobject.h"

namespace LoginWindow {

// Drawing and managing captcha with slider
class CaptchaItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit CaptchaItem(QGraphicsObject *parent, const QString &background, const QString &slider, int top);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setClickable(bool enabled);
    void updateScaling() override;

    void setCaptcha();

signals:
    void captchaResolved(const QString &captchaSolution, const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    QString backgroundBase64_;
    QString sliderBase64_;
    QPixmap backgroundPixmap_;
    QPixmap sliderPixmap_;
    double pixmapScale_;
    int sliderLeft_ = 0;
    int sliderTop_ = 0;

    std::optional<QPointF> mousePressedPos_;

    static const int kMaxTrailSize = 1000;
    std::vector<float> captchaTrailX_;
    std::vector<float> captchaTrailY_;

    bool inCaptchaArea(const QPointF &pt) const;
    bool inSliderArea(const QPointF &pt) const;
    void recreatePixmaps();

    QRectF sliderBackgroundRect() const;
    QRectF sliderRunnerRect() const;
};

}
