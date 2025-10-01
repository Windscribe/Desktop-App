#pragma once

#include <QGraphicsObject>
#include <QFont>
#include "commongraphics/iconbutton.h"

namespace ConnectWindow {

class LogoNotificationsButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit LogoNotificationsButton(ScalableGraphicsObject * parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setCountState(int countAll, int countUnread);
    void setIsPremium(bool isPremium);

private slots:
    void onHoverEnter();
    void onHoverLeave();

private:
    static constexpr int WIDTH = 168;
    static constexpr int HEIGHT = 36;

    QColor numberColor_;
    QColor backgroundFillColor_;
    QColor backgroundOutlineColor_;

    static constexpr qreal OPACITY_ALL_READ = .24;
    qreal curBackgroundOpacity_;

    QString numNotifications_;

    static constexpr int MARGIN = 4;

    int unread_;
    bool isPremium_;
};

}
