#pragma once

#include <QGraphicsObject>
#include "commongraphics/baseitem.h"

namespace PreferencesWindow {

class StaticText : public CommonGraphics::BaseItem
{
    Q_OBJECT

public:
    explicit StaticText(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setCaption(const QString &caption);
    void setText(const QString &text);

    void updateScaling() override;

private:
    QString caption_;
    QString text_;
};

} // namespace PreferencesWindow
