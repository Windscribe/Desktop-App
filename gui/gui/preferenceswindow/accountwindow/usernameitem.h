#ifndef USERNAMEITEM_H
#define USERNAMEITEM_H

#include <QGraphicsObject>
#include "../baseitem.h"
#include "../dividerline.h"


namespace PreferencesWindow {

class UsernameItem : public BaseItem
{
    Q_OBJECT
public:
    explicit UsernameItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setUsername(const QString &username);
    virtual void updateScaling();

private:
    QString username_;
    DividerLine *dividerLine_;
};

} // namespace PreferencesWindow

#endif // USERNAMEITEM_H
