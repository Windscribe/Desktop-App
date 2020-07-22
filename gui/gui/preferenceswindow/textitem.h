#ifndef TEXTITEM_H
#define TEXTITEM_H

#include "baseitem.h"

namespace PreferencesWindow {


class TextItem : public BaseItem
{
public:
    explicit TextItem(ScalableGraphicsObject *parent, QString text, int height);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setText(QString text);
    virtual void updateScaling();

private:
    QString text_;
    int height_;
};

}
#endif // TEXTITEM_H
