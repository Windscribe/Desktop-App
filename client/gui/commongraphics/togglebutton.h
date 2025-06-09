#pragma once

#include <QColor>
#include <QGraphicsObject>
#include <QVariantAnimation>
#include "clickablegraphicsobject.h"

class ToggleButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit ToggleButton(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setState(bool isChecked);
    bool isChecked() const;
    void setEnabled(bool enabled);
    void setColor(const QColor &color);

    void setToggleable(bool toggleable);

signals:
    void stateChanged(bool isChecked);
    void toggleIgnored();

protected:
    static constexpr int TOGGLE_RADIUS = 12;
    static constexpr int BUTTON_OFFSET = 2;
    static constexpr int BUTTON_WIDTH = 18;
    static constexpr int BUTTON_HEIGHT = 18;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onOpacityChanged(const QVariant &value);

private:
    double animationProgress_;
    bool isChecked_;
    bool isToggleable_;
    bool enabled_;
    QColor color_;
    QVariantAnimation opacityAnimation_;
};
