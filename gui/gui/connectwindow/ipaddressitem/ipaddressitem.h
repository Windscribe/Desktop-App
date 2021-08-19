#ifndef IPADDRESSITEM_H
#define IPADDRESSITEM_H

#include <QObject>
#include "numberspixmap.h"
#include "octetitem.h"

class IPAddressItem : public QObject
{
    Q_OBJECT
public:
    explicit IPAddressItem(QObject *parent = nullptr);

    void setIpAddress(const QString &ip, bool bWithAnimation = true);

    int width() const;
    int height() const;

    void draw(QPainter *painter, int x, int y);

    void setScale(double scale);

signals:
    void needUpdate();
    void widthChanged(int newWidth);


private slots:
    void onOctetWidthChanged();

private:
    NumbersPixmap numbersPixmap_;
    OctetItem *octetItems_[4];
    //NumberItem *numberItem_[12];
    int curWidth_;

    QString ipAddress_;
    bool isValid_;
};

#endif // IPADDRESSITEM_H
