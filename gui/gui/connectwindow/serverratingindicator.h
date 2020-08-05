#ifndef PINGBARBUTTON_H
#define PINGBARBUTTON_H

#include <QGraphicsObject>
#include "../backend/types/types.h"
#include "../backend/types/pingtime.h"
#include "commongraphics/clickablegraphicsobject.h"

namespace ConnectWindow {

class ServerRatingIndicator : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit ServerRatingIndicator(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void onConnectStateChanged(ProtoTypes::ConnectStateType newConnectState, ProtoTypes::ConnectStateType prevConnectState);

    void setPingTime(const PingTime &pingTime);

    void updateScaling() override;

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
