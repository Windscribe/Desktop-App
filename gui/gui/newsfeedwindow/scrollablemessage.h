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

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setMessage(const QString &strMessage);
    void updateScaling() override;

protected:
    bool eventFilter(QObject *watching, QEvent *event) override;

private:
    int height_;
    int width_;

    QTextBrowser *textBrowser_;
    QGraphicsProxyWidget *proxyWidget_;

    void updatePositions();
};

#endif // SCROLLABLEMESSAGE_H
