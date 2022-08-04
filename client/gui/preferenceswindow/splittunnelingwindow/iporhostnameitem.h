#ifndef IPORHOSTNAMEITEM_H
#define IPORHOSTNAMEITEM_H

#include "../baseitem.h"
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class IpOrHostnameItem : public BaseItem
{
    Q_OBJECT
public:
    explicit IpOrHostnameItem(types::SplitTunnelingNetworkRoute route, ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString getIpOrHostnameText();
    types::SplitTunnelingNetworkRoute getNetworkRoute();

    void setSelected(bool selected) override;
    void updateScaling() override;

signals:
    void deleteClicked();

private slots:
    void onDeleteButtonHoverEnter();

private:
    types::SplitTunnelingNetworkRoute route_;
    QString text_;
    IconButton *deleteButton_;
    DividerLine *line_;

    void updatePositions();
};

}

#endif // IPORHOSTNAMEITEM_H
