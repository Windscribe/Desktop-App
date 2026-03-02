#pragma once

#include <QVariantAnimation>
#include <QGraphicsSceneHoverEvent>
#include "graphicresources/fontdescr.h"
#include "clickablegraphicsobject.h"

namespace CommonGraphics {

class TextWrapIconButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    TextWrapIconButton(int spacerWidth, const QString text, const QString &imagePath, ScalableGraphicsObject *parent, bool bSetClickable);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setFont(const FontDescr &fontDescr);

    void animateHide();
    void animateShow();

    const QString text();
    void setText(QString text);

    void setVerticalOffset(int offset);

    void setMaxWidth(int width);
    int getWidth();
    int getHeight();

    void updateScaling() override;

    void setOpacityByFactor(double opacityFactor);

    void setImagePath(const QString &imagePath);

signals:
    void widthChanged(int newWidth);
    void heightChanged(int newHeight);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onTextOpacityChanged(const QVariant &value);
    void onIconOpacityChanged(const QVariant &value);

private:
    void recalcWidth();
    void recalcHeight();

    int width_;
    int maxWidth_;
    int height_;

    int spacerWidth_;

    QString iconPath_;
    QString text_;

    double curTextOpacity_;
    double curIconOpacity_;

    FontDescr fontDescr_;
    int yOffset_;

    QVariantAnimation textOpacityAnimation_;
    QVariantAnimation iconOpacityAnimation_;
};

}
