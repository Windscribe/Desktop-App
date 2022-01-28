#ifndef IPORHOSTNAMEITEM_H
#define IPORHOSTNAMEITEM_H

#include "../baseitem.h"
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"
#include "utils/protobuf_includes.h"

namespace PreferencesWindow {

class IpOrHostnameItem : public BaseItem
{
    Q_OBJECT
public:
    explicit IpOrHostnameItem(ProtoTypes::SplitTunnelingNetworkRoute route, ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString getIpOrHostnameText();
    ProtoTypes::SplitTunnelingNetworkRoute getNetworkRoute();

    void setSelected(bool selected) override;
    void updateScaling() override;

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
