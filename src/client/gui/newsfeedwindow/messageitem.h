#pragma once

#include <QFont>
#include <QString>
#include <QTextBrowser>
#include <QGraphicsProxyWidget>
#include "commongraphics/scalablegraphicsobject.h"

namespace NewsFeedWindow {

class MessageItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit MessageItem(QGraphicsObject *parent, int width, QString msg);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

protected:
    bool eventFilter(QObject *watching, QEvent *event) override;

private:
    void updatePositions();
    QString msg_;

    int width_;
    QTextBrowser *textBrowser_;
    QGraphicsProxyWidget *proxyWidget_;
};

} // namespace NewsFeedWindow
