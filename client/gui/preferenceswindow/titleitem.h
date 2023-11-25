#pragma once

#include <QGraphicsObject>
#include "commongraphics/baseitem.h"

namespace PreferencesWindow {

class TitleItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit TitleItem(ScalableGraphicsObject *parent, const QString &title = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setTitle(const QString &title);

private:
    static constexpr int ACCOUNT_TITLE_HEIGHT = 14;
    QString title_;
};

} // namespace PreferencesWindow
