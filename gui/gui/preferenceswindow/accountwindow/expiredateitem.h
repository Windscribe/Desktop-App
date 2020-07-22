#ifndef EXPIREDATEITEM_H
#define EXPIREDATEITEM_H

#include "../baseitem.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class ExpireDateItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ExpireDateItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);
    virtual void updateScaling();

public:
    void setDate(const QString &date);

private:
    QString dateStr_;
    DividerLine *dividerLine_;
};

} // namespace PreferencesWindow

#endif // EXPIREDATEITEM_H
