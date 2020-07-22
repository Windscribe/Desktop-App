#ifndef APPITEM_H
#define APPITEM_H

#include "../baseitem.h"
#include "../dividerline.h"
#include "CommonGraphics/iconbutton.h"
#include "IPC/generated_proto/types.pb.h"

namespace PreferencesWindow {

class AppSearchItem : public BaseItem
{
    Q_OBJECT
public:
    explicit AppSearchItem(ProtoTypes::SplitTunnelingApp app, QString appIconPath, ScalableGraphicsObject *parent);
    ~AppSearchItem();
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    QString getName();
    QString getFullName();
    QString getAppIcon();
    void setAppIcon(QString icon);

    void updateIcons();

    void setSelected(bool selected);
    virtual void updateScaling();

private slots:
    void onTextOpacityChanged(const QVariant &value);
    void onHoverEnter();

private:
    double textOpacity_;
    ProtoTypes::SplitTunnelingApp app_;

    QString appIcon_;

    double enabledIconOpacity_;
    QString enabledIcon_;

    DividerLine *line_;

    QVariantAnimation textOpacityAnimation_;

};

}
#endif // APPITEM_H
