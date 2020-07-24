#ifndef BUBBLEBUTTONDARK_H
#define BUBBLEBUTTONDARK_H

#include <QGraphicsObject>
#include <QColor>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QFont>
#include "commongraphics/clickablegraphicsobject.h"
#include "graphicresources/fontdescr.h"


namespace CommonGraphics {


class BubbleButtonDark : public ClickableGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
public:
    explicit BubbleButtonDark(ScalableGraphicsObject *parent, int width, int height, int arcWidth, int arcHeight);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void animateColorChange(QColor textColor, QColor outlineColor, int animationSpeed);
    void animateOpacityChange(double newOutlineOpacity, double newTextOpacity, int animationSpeed);
    void animateHide(int animationSpeed);
    void animateShow(int animationSpeed);

    void quickHide();
    void quickShow();

    void setText(QString text);

    QColor fillColor() {return curFillColor_; }
    QColor textColor() {return curTextColor_; }

    void setFillColor(QColor newFillColor);
    void setTextColor(QColor newTextColor);

    void setFont(const FontDescr &fontDescr);

private slots:
    void onOutlineOpacityChanged(const QVariant &value);
    void onTextOpacityChanged(const QVariant &value);

    void onTextColorChanged(const QVariant &value);
    void onFillColorChanged(const QVariant &value);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    int width_ = 108;
    int height_ = 40;

    int arcWidth_ = 20;
    int arcHeight_ = 20;


    FontDescr fontDescr_;
    QString text_;

    double curOutlineFillOpacity_;
    double curTextOpacity_;
    QColor curOutlineColor_;
    QColor curFillColor_;
    QColor curTextColor_;

    QVariantAnimation outlineOpacityAnimation_;
    QVariantAnimation textOpacityAnimation_;

    QPropertyAnimation textColorAnimation_;
    QPropertyAnimation fillColorAnimation_;

};

}

#endif // BUBBLEBUTTONDARK_H
