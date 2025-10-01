#pragma once

#include <QEvent>
#include <QGraphicsProxyWidget>
#include <QTextBrowser>
#include <QVariantAnimation>
#include <QWidget>

#include "commongraphics/scalablegraphicsobject.h"

namespace EmergencyConnectWindow {

class TextLinkItem : public QTextBrowser
{
    Q_OBJECT

public:
    explicit TextLinkItem(QGraphicsObject *parent);

    void updateScaling();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onLinkOpacityChanged(const QVariant &value);
    void onLanguageChanged();

private:
    QGraphicsProxyWidget *proxy_;
    QString text_;

    int width_;
    int height_;
    int linkBegin_;
    int linkEnd_;

    QVariantAnimation linkOpacityAnimation_;
    double linkOpacity_;
};

}
