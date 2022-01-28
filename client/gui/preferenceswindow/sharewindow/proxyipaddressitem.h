#ifndef PROXYIPADDRESSITEM_H
#define PROXYIPADDRESSITEM_H

#include <QGraphicsObject>
#include <QTimer>
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"
#include "tooltips/tooltiptypes.h"

namespace PreferencesWindow {

class ProxyIpAddressItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit ProxyIpAddressItem(ScalableGraphicsObject *parent, bool isDrawFullBottomDivider);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setIP(const QString &strIP);
    void cancelToolTip();

    void updateScaling() override;

signals:
    void textChanged(const QString &text);

private slots:
    void onCopyClick();

private:
    QString strIP_;
    IconButton *btnCopy_;
    DividerLine *line_;

    void updatePositions();
};

} // namespace PreferencesWindow


#endif // PROXYIPADDRESSITEM_H
