#ifndef APPINCLUDEDITEM_H
#define APPINCLUDEDITEM_H

#include "../baseitem.h"
#include "../dividerline.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/iconbutton.h"
#include "ipc/generated_proto/types.pb.h"

namespace PreferencesWindow {

class AppIncludedItem : public BaseItem
{
    Q_OBJECT
public:
    explicit AppIncludedItem(ProtoTypes::SplitTunnelingApp app, QString iconPath, ScalableGraphicsObject *parent);
    ~AppIncludedItem();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    QString getName();
    QString getAppIcon();

    void setSelected(bool selected) override;
    void updateScaling() override;

signals:
    void deleteClicked();

private slots:
    void onDeleteButtonHoverEnter();

private:
    QString appIcon_;
    ProtoTypes::SplitTunnelingApp app_;
    IconButton *deleteButton_;

    DividerLine *line_;

    void updatePositions();
};

}

#endif // APPINCLUDEDITEM_H
