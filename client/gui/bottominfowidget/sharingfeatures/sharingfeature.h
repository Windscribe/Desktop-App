#pragma once

#include <QGraphicsObject>
#include <QFont>
#include <QVariantAnimation>
#include "commongraphics/dividerline.h"
#include "commongraphics/texticonbutton.h"
#include "commongraphics/verticaldividerline.h"

namespace SharingFeatures {

class SharingFeature : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit SharingFeature(QString ssidOrProxyText, QString primaryIcon, ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(QString text);
    void setPrimaryIcon(QString iconPath);
    void setNumber(int number);

    void setOpacityByFactor(double opacityFactor);

    void setClickable(bool clickable);

    void setRounded(bool rounded);

    void updateScaling() override;

    static constexpr int HEIGHT = 48;

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

    static constexpr int WIDTH = 350;

    static constexpr int DIVIDER_POS_X = 56;
    static constexpr int SECONDARY_ICON_POS_X = DIVIDER_POS_X + 16;
    static constexpr int USER_NUMBER_POS_X = SECONDARY_ICON_POS_X + 16 + 8;
    static constexpr int SPACE_BETWEEN_TEXT_AND_ARROW = 4;

    double curDefaultOpacity_;

    void updatePositions();

};

}
