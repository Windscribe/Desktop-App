#pragma once

#include "commongraphics/baseitem.h"

namespace PreferencesWindow {

class AccountDataItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit AccountDataItem(ScalableGraphicsObject *parent, const QString &value1, const QString &value2);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setValue1(const QString &value);
    void setValue2(const QString &value);

public:

private:
    QString value1_;
    QString value2_;
};

} // namespace PreferencesWindow
