#ifndef MIDDLEITEM_H
#define MIDDLEITEM_H

#include <QFont>
#include "commongraphics/scalablegraphicsobject.h"
#include "ipaddressitem/ipaddressitem.h"

namespace ConnectWindow {

// item with "Firewall" text, IP-address and SECURE_LOCK_ICON
class MiddleItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit MiddleItem(ScalableGraphicsObject *parent, const QString &ipAddress);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setTextColor(QColor color);

    void updateScaling() override;

public slots:
    void setIpAddress(const QString &ipAddress);
    void setIsSecured(bool isSecured);

private slots:
    void onUpdate();

private:
    QString ipAddress_;
    bool isSecured_;
    QColor curTextColor_;
    IPAddressItem *ipAddressItem_;
};

} //namespace ConnectWindow

#endif // MIDDLEITEM_H
