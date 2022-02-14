#ifndef TEXTITEM_H
#define TEXTITEM_H

#include "baseitem.h"

namespace PreferencesWindow {


class TextItem : public BaseItem
{
public:
    TextItem(ScalableGraphicsObject *parent, QString text, int height);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(QString text);
    void updateScaling() override;

private:
    QString text_;
    int local_height_;
};

}
#endif // TEXTITEM_H
