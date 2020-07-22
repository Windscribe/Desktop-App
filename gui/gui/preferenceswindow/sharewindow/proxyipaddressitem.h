#ifndef PROXYIPADDRESSITEM_H
#define PROXYIPADDRESSITEM_H

#include <QGraphicsObject>
#include <QTimer>
#include "CommonGraphics/iconbutton.h"
#include "../dividerline.h"
#include "Tooltips/tooltiptypes.h"

namespace PreferencesWindow {

class ProxyIpAddressItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit ProxyIpAddressItem(ScalableGraphicsObject *parent, bool isDrawFullBottomDivider);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setIP(const QString &strIP);
    void cancelToolTip();

    virtual void updateScaling();

signals:
    void textChanged(const QString &text);
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

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
