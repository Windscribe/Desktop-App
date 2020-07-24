#ifndef NOTIFICCATIONLABEL_H
#define NOTIFICCATIONLABEL_H

#include <QGraphicsObject>
#include <QFont>
#include "commongraphics/iconbutton.h"

namespace ConnectWindow {

class LogoNotificationsButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit LogoNotificationsButton(ScalableGraphicsObject * parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setCountState(int countAll, int countUnread);

private slots:
    void onHoverEnter();
    void onHoverLeave();

private:
    const int WIDTH = 140;
    const int HEIGHT = 36;

    QColor numberColor_;
    QColor backgroundFillColor_;
    QColor backgroundOutlineColor_;

    const qreal OPACITY_ALL_READ = .24;
    qreal curBackgroundOpacity_;

    QString numNotifications_;

    const int MARGIN = 4;

    int unread_;
};

}

#endif // NOTIFICCATIONLABEL_H
