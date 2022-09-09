#ifndef CHANGELOGITEM_H
#define CHANGELOGITEM_H

#include <QString>
#include "commongraphics/baseitem.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

class ChangelogItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit ChangelogItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

private:
    void updatePositions();
    QString text_;
};

} // namespace PreferencesWindow

#endif // CHANGELOGITEM_H