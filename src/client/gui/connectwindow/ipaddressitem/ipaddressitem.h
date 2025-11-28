#pragma once

#include "commongraphics/scalablegraphicsobject.h"
#include "numberspixmap.h"
#include "octetitem.h"

#include <QGraphicsBlurEffect>
#include <QGraphicsSceneMouseEvent>

class IPAddressItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit IPAddressItem(ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString ipAddress();
    void setIpAddress(const QString &ip, bool bWithAnimation = true);
    void updateScaling() override;

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onOctetWidthChanged();
    void doUpdate();

signals:
    void widthChanged(int width);

private:
    NumbersPixmap numbersPixmap_;
    OctetItem *octetItems_[4];
    QGraphicsBlurEffect blurEffect_;
    int curWidth_;

    QString ipAddress_;
    bool isValid_;

    const int defaultBlurRadius_ = 16;
};
