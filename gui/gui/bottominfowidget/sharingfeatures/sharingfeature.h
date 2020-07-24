#ifndef SHARINGFEATURE_H
#define SHARINGFEATURE_H

#include <QGraphicsObject>
#include <QFont>
#include <QVariantAnimation>
#include "commongraphics/texticonbutton.h"
#include "preferenceswindow/dividerline.h"
#include "commongraphics/verticaldividerline.h"

namespace SharingFeatures {

class SharingFeature : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit SharingFeature(QString ssidOrProxyText, QString primaryIcon, ScalableGraphicsObject *parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setText(QString text);
    void setPrimaryIcon(QString iconPath);
    void setNumber(int number);

    void setOpacityByFactor(double opacityFactor);

    void setClickable(bool clickable);

    void setRounded(bool rounded);

    virtual void updateScaling();

    static const int HEIGHT = 48;

signals:
    void clicked();

private slots:
    void onTextIconWidthChanged(int newWidth);

private:
    CommonGraphics::TextIconButton *textIconButton_;

    CommonGraphics::VerticalDividerLine *verticalLine_;

    QString primaryIcon_;
    QString secondaryIcon_;
    int number_;

    bool rounded_;

    const int WIDTH = 332;

    const int DIVIDER_POS_X = 56;
    const int SECONDARY_ICON_POS_X = DIVIDER_POS_X + 16;
    const int USER_NUMBER_POS_X = SECONDARY_ICON_POS_X + 16 + 8;

    double curDefaultOpacity_;

    const int SPACE_BETWEEN_TEXT_AND_ARROW = 3;

    void updatePositions();

};

}
#endif // SHARINGFEATURE_H
