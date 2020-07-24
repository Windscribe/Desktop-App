#ifndef WIFINAME_H
#define WIFINAME_H

#include <QFont>
#include <QGraphicsObject>
#include "commongraphics/texticonbutton.h"

namespace ConnectWindow {

class WiFiName : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit WiFiName(ScalableGraphicsObject *parent, const QString &wifiName);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void setClickable(bool clickable);

signals:
    void clicked();

public slots:
    void setWiFiName(const QString &name);

private:
    QString wifiName_;
    QFont font_;
    QFont fontTrust_;

    CommonGraphics::TextIconButton *trustButton_;
    const int SPACE_BETWEEN_TEXT_AND_ARROW = 5;

    int width_ = 320;
};

} //namespace ConnectWindow

#endif // WIFINAME_H
