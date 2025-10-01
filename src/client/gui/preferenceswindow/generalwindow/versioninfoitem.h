#pragma once

#include "commongraphics/baseitem.h"
#include <QMenu>

namespace PreferencesWindow {

class VersionInfoItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit VersionInfoItem(ScalableGraphicsObject *parent, const QString &caption, const QString &value);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setCaption(const QString &caption);

private:
    QString strCaption_;
    QString strValue_;
};

} // namespace PreferencesWindow
