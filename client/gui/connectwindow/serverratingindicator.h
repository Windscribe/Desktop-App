#ifndef PINGBARBUTTON_H
#define PINGBARBUTTON_H

#include <QGraphicsObject>
#include "types/pingtime.h"
#include "types/connectstate.h"
#include "commongraphics/clickablegraphicsobject.h"
#include "utils/imagewithshadow.h"

namespace ConnectWindow {

class ServerRatingIndicator : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit ServerRatingIndicator(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void onConnectStateChanged(CONNECT_STATE newConnectState, CONNECT_STATE prevConnectState);

    void setPingTime(const PingTime &pingTime);

    void updateScaling() override;

signals:
    void clicked();

private:
    int width_;
    int height_;
    CONNECT_STATE connectState_;
    PingTime pingTime_;

    QScopedPointer<ImageWithShadow> pingOnFull_;
    QScopedPointer<ImageWithShadow> pingOnLow_;
    QScopedPointer<ImageWithShadow> pingOnHalf_;
    QScopedPointer<ImageWithShadow> pingOffFull_;
    QScopedPointer<ImageWithShadow> pingOffLow_;
    QScopedPointer<ImageWithShadow> pingOffHalf_;

    void updateDimensions();
};

} //namespace ConnectWindow

#endif // PINGBARBUTTON_H
