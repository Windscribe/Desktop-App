#ifndef TEXTICONBUTTON_H
#define TEXTICONBUTTON_H

#include <QVariantAnimation>
#include <QGraphicsSceneHoverEvent>
#include "graphicresources/fontdescr.h"
#include "clickablegraphicsobject.h"

namespace CommonGraphics {

class TextIconButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    TextIconButton(int spacerWidth, const QString text, const QString &imagePath, ScalableGraphicsObject *parent, bool bSetClickable);
    virtual QRectF boundingRect() const;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setFont(const FontDescr &fontDescr);

    void animateHide();
    void animateShow();

    void setText(QString text);

    int getWidth();
    int getHeight();

    virtual void updateScaling();

    void setOpacityByFactor(double opacityFactor);

signals:
    void widthChanged(int newWidth);
    void heightChanged(int newHeight);

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private slots:
    void onTextOpacityChanged(const QVariant &value);
    void onIconOpacityChanged(const QVariant &value);

private:
    void recalcWidth();
    void recalcHeight();

    int width_;
    int height_;

    int spacerWidth_;
    QString spacer();

    QString iconPath_;
    QString text_;

    double curTextOpacity_;
    double curIconOpacity_;

    FontDescr fontDescr_;

    QVariantAnimation textOpacityAnimation_;
    QVariantAnimation iconOpacityAnimation_;

    const int MARGIN_X = 0;
    const int MARGIN_Y = 0;

    const int PIXMAP_WIDTH = 16;
};

}

#endif // TEXTICONBUTTON_H
