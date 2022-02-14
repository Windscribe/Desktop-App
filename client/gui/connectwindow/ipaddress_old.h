#ifndef IPADDRESS_H
#define IPADDRESS_H

#include <QFont>
#include <QGraphicsObject>

namespace ConnectWindow {

class IPAddress : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit IPAddress(QGraphicsObject *parent, const QString &ipAddress);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setTextColor(QColor color);
    void setLockColorDark(bool dark);

public slots:
    void setIpAddress(const QString &ipAddress);
    void setIsSecured(bool isSecured);

private:
    QString ipAddress_;
    bool isSecured_;
    QFont font_;

    QColor curTextColor_;
    bool lockColorDark_;
};

} //namespace ConnectWindow

#endif // IPADDRESS_H
