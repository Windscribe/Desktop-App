#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"

namespace PreferencesWindow {

class DnsDomainAddressItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit DnsDomainAddressItem(const QString &address, ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QString getAddressText();

    void setSelected(bool selected) override;
    void updateScaling() override;

    void setClickable(bool clickable) override;

signals:
    void deleteClicked();

private slots:
    void onDeleteButtonHoverEnter();

private:
    QString address_;
    IconButton *deleteButton_;

    void updatePositions();
};

}

