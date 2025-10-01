#pragma once

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"

namespace PreferencesWindow {

class MacAddressItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit MacAddressItem(ScalableGraphicsObject *parent, const QString &caption = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setCaption(const QString &caption);
    void setMacAddress(const QString &macAddress);
    void updateScaling() override;

signals:
    void cycleMacAddressClick();

private slots:
    void onUpdateClick();

private:
    void updatePositions();

    QString caption_;
    QString text_;
    IconButton *updateButton_;
    bool isEditMode_;
};

} // namespace PreferencesWindow
