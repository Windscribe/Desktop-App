#pragma once

#include <QFont>
#include <QColor>
#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include "commongraphics/clickablegraphicsobject.h"

namespace LoginWindow {

class LoginYesNoButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:

    explicit LoginYesNoButton(QString text, ScalableGraphicsObject * parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(QString text);

    bool isHighlighted();

    void animateButtonHighlight();
    void animateButtonUnhighlight();

    void setOpacityByFactor(double newOpacityFactor);

signals:
    void activated();
    void deactivated();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onTextHoverOpacityChanged(const QVariant &value);
    void onIconHoverOpacityChanged(const QVariant &value);
    void onTextHoverOffsetChanged(const QVariant &value);

private:
    int width_;
    int height_;

    QString text_;
    bool highlight_;

    double curTextOpacity_;
    QVariantAnimation textOpacityAnimation_;

    double curIconOpacity_;
    QVariantAnimation iconOpacityAnimation_;

    int curTextPosX_;
    QVariantAnimation textPosXAnimation_;

    static constexpr int MARGIN_UNHOVER_TEXT_PX = 16;
    static constexpr int MARGIN_HOVER_TEXT_PX   = 24;
};

} // namespace LoginWindow
