#pragma once

#include <QGraphicsObject>
#include <QTimer>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "tooltips/tooltiptypes.h"

namespace PreferencesWindow {

class ProxyIpAddressItem : public CommonGraphics::BaseItem
{
    Q_OBJECT

public:
    explicit ProxyIpAddressItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setIP(const QString &strIP);
    void cancelToolTip();

    void updateScaling() override;

signals:
    void textChanged(const QString &text);

private slots:
    void onCopyClick();

private:
    void updatePositions();

    QString strIP_;
    IconButton *copyButton_;
};

} // namespace PreferencesWindow
