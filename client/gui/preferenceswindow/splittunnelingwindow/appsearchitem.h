#ifndef APPITEM_H
#define APPITEM_H

#include "../baseitem.h"
#include "../dividerline.h"
#include "commongraphics/iconbutton.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class AppSearchItem : public BaseItem
{
    Q_OBJECT
public:
    AppSearchItem(types::SplitTunnelingApp app, QString appIconPath, ScalableGraphicsObject *parent);
    ~AppSearchItem();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString getName();
    QString getFullName();
    QString getAppIcon();
    void setAppIcon(QString icon);

    void updateIcons();

    void setSelected(bool selected) override;
    void updateScaling() override;

private slots:
    void onTextOpacityChanged(const QVariant &value);
    void onHoverEnter();

private:
    double textOpacity_;
    types::SplitTunnelingApp app_;

    QString appIcon_;

    double enabledIconOpacity_;
    QString enabledIcon_;

    DividerLine *line_;

    QVariantAnimation textOpacityAnimation_;

};

}
#endif // APPITEM_H
