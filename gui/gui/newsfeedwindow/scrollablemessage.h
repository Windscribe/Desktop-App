#ifndef SCROLLABLEMESSAGE_H
#define SCROLLABLEMESSAGE_H

#include <QString>
#include <QFont>
#include <QTextBrowser>
#include <QGraphicsProxyWidget>
#include "commongraphics/scalablegraphicsobject.h"

class ScrollableMessage : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ScrollableMessage(int width, int height, QGraphicsObject *parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setMessage(const QString &strMessage);
    void updateScaling();

protected:
    bool eventFilter(QObject *watching, QEvent *event);

private:
    int height_;
    int width_;

    QTextBrowser *textBrowser_;
    QGraphicsProxyWidget *proxyWidget_;

    void updatePositions();
};

#endif // SCROLLABLEMESSAGE_H
