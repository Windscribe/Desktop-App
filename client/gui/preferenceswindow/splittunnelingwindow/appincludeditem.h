#ifndef APPINCLUDEDITEM_H
#define APPINCLUDEDITEM_H

#include "../baseitem.h"
#include "../dividerline.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/iconbutton.h"
#include "utils/protobuf_includes.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class AppIncludedItem : public BaseItem
{
    Q_OBJECT
public:
    explicit AppIncludedItem(types::SplitTunnelingApp app, QString iconPath, ScalableGraphicsObject *parent);
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
    types::SplitTunnelingApp app_;
    IconButton *deleteButton_;

    DividerLine *line_;

    void updatePositions();
};

}

#endif // APPINCLUDEDITEM_H
