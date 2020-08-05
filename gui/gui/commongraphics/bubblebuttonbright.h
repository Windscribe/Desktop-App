#ifndef BUBBLEBUTTONBRIGHT_H
#define BUBBLEBUTTONBRIGHT_H

#include <QGraphicsObject>
#include <QColor>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QFont>
#include "clickablegraphicsobject.h"
#include "graphicresources/fontdescr.h"

namespace CommonGraphics {

class BubbleButtonBright : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    BubbleButtonBright(ScalableGraphicsObject *parent, int width, int height, int arcWidth, int arcHeight);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void animateOpacityChange(double newOutlineOpacity, double newTextOpacity, int animationSpeed);
    void animateHide(int animationSpeed);
    void animateShow(int animationSpeed);

    void quickHide();
    void quickShow();

    void setText(QString text);

    void setFont(const FontDescr &fontDescr);

    void setTextColor(QColor color);

    double curFillOpacity();
    void setFillOpacity(double opacity);

private slots:
    void onOutlineOpacityChanged(const QVariant &value);
    void onTextOpacityChanged(const QVariant &value);

    void onButtonColorChanged(const QVariant &value);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    int width_ = 128;
    int height_ = 40;

    int arcWidth_ = 20;
    int arcHeight_ = 20;

    FontDescr fontDescr_;

    QString text_;

    double curOutlineFillOpacity_;
    double curTextOpacity_;
    QColor curFillColor_;
    QColor curTextColor_;

    QVariantAnimation outlineOpacityAnimation_;
    QVariantAnimation textOpacityAnimation_;

    QPropertyAnimation outlineHoverAnimation_;
};

}

#endif // BUBBLEBUTTONBRIGHT_H
