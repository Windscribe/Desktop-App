#ifndef PINGBARBUTTON_H
#define PINGBARBUTTON_H

#include <QGraphicsObject>
#include "../Backend/Types/types.h"
#include "../Backend/Types/pingtime.h"
#include "CommonGraphics/clickablegraphicsobject.h"

namespace ConnectWindow {

class ServerRatingIndicator : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit ServerRatingIndicator(ScalableGraphicsObject *parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void onConnectStateChanged(ProtoTypes::ConnectStateType newConnectState, ProtoTypes::ConnectStateType prevConnectState);

    void setPingTime(const PingTime &pingTime);

    virtual void updateScaling();

signals:
    void clicked();

private:
    int width_;
    int height_;
    ProtoTypes::ConnectStateType connectState_;
    PingTime pingTime_;

    void updateDimensions();
};

} //namespace ConnectWindow

#endif // PINGBARBUTTON_H
