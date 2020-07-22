#ifndef IPORHOSTNAMEITEM_H
#define IPORHOSTNAMEITEM_H

#include "../baseitem.h"
#include "CommonGraphics/iconbutton.h"
#include "../dividerline.h"
#include "IPC/generated_proto/types.pb.h"

namespace PreferencesWindow {

class IpOrHostnameItem : public BaseItem
{
    Q_OBJECT
public:
    explicit IpOrHostnameItem(ProtoTypes::SplitTunnelingNetworkRoute route, ScalableGraphicsObject *parent);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    QString getIpOrHostnameText();
    ProtoTypes::SplitTunnelingNetworkRoute getNetworkRoute();

    void setSelected(bool selected);
    virtual void updateScaling();

signals:
    void deleteClicked();

private slots:
    void onDeleteButtonHoverEnter();

private:
    ProtoTypes::SplitTunnelingNetworkRoute route_;
    QString text_;
    IconButton *deleteButton_;
    DividerLine *line_;

    void updatePositions();
};

}

#endif // IPORHOSTNAMEITEM_H
