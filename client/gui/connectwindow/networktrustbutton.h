#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>

#include "commongraphics/clickablegraphicsobject.h"
#include "commongraphics/imageitem.h"
#include "types/networkinterface.h"

namespace ConnectWindow {

class NetworkTrustButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit NetworkTrustButton(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setNetwork(const types::NetworkInterface &network);
    void updateScaling() override;

private slots:
    void onOpacityValueChanged(const QVariant &value);
    void onHoverEnter();
    void onHoverLeave();

private:
    static const int kMaxWidth = 198;

    types::NetworkInterface network_;
    ImageItem *trustIcon_;
    ImageItem *arrow_;

    QVariantAnimation opacityAnimation_;
    double animProgress_;

    QString networkText_;
    int width_;
    double arrowShift_;
    bool isTextElided_;

    void updateNetworkText();
    void updatePositions();
};

} //namespace ConnectWindow
