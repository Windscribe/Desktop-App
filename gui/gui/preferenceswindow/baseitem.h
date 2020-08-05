#ifndef BASEITEM_H
#define BASEITEM_H

#include <QGraphicsObject>
#include "commongraphics/clickablegraphicsobject.h"

namespace PreferencesWindow {

class BaseItem : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit BaseItem(ScalableGraphicsObject *parent, int height, QString id = "", bool clickable = false);
    QRectF boundingRect() const override;

    void setHeight(int height);

    virtual void hideOpenPopups();

    QString id();
    void setId(QString id);

signals:
    void heightChanged(int newHeight);

protected:
    int height_;
    QString id_;

};

} // namespace PreferencesWindow


#endif // BASEITEM_H
