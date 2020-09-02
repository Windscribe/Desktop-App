#ifndef IUPDATEAPPITEM_H
#define IUPDATEAPPITEM_H

#include <QGraphicsObject>

class IUpdateAppItem
{
public:
    virtual ~IUpdateAppItem() {}

    virtual QGraphicsObject * getGraphicsObject() = 0;

    virtual void setVersionAvailable(const QString &versionNumber) = 0;
    virtual void setProgress(int value) = 0; // from 0 to 100

    virtual QPixmap getCurrentPixmapShape() = 0;

    virtual void updateScaling() = 0;

    enum UpdateAppItemMode { UPDATE_APP_ITEM_MODE_PROMPT, UPDATE_APP_ITEM_MODE_PROGRESS };
    virtual void setMode(UpdateAppItemMode mode) = 0;

signals:
    void updateClick();

};

Q_DECLARE_INTERFACE(IUpdateAppItem, "IUpdateAppItem")


#endif // IUPDATEAPPITEM_H
