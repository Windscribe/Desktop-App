#pragma once

#include <QGraphicsObject>
#include <QColor>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QFont>
#include "commongraphics/clickablegraphicsobject.h"
#include "graphicresources/fontdescr.h"

namespace CommonGraphics {

class BubbleButton: public ClickableGraphicsObject
{
    Q_OBJECT
public:
    enum Style { kBright, kDark, kOutline };
    BubbleButton(ScalableGraphicsObject *parent, Style style, int width, int height, int radius);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void animateHide(int animationSpeed);
    void animateShow(int animationSpeed);

    void quickHide();
    void quickShow();

    void setText(QString text);

    void setFillColor(QColor newFillColor);
    void setTextColor(QColor newTextColor);

    void setFont(const FontDescr &fontDescr);
    void setWidth(int width);

    void setStyle(Style style);

    void hover();
    void unhover();

private slots:
    void onTextColorChanged(const QVariant &value);
    void onFillColorChanged(const QVariant &value);
    void onTextOpacityChanged(const QVariant &value);
    void onOutlineOpacityChanged(const QVariant &value);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    Style style_;

    int width_;
    int height_;
    int radius_;

    FontDescr fontDescr_;
    QString text_;

    double curOutlineFillOpacity_;
    double curTextOpacity_;
    QColor curFillColor_;
    QColor curTextColor_;
    QColor fillColor_;
    QColor textColor_;

    QVariantAnimation outlineOpacityAnimation_;
    QVariantAnimation textOpacityAnimation_;
    QVariantAnimation textColorAnimation_;
    QVariantAnimation fillColorAnimation_;

    bool showOutline_;
};

} // namespace CommonGraphics