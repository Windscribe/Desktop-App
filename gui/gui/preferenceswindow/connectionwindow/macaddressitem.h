#ifndef MACADDRESSITEM_H
#define MACADDRESSITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QRegExpValidator>
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class MacAddressItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit MacAddressItem(ScalableGraphicsObject *parent, const QString &caption);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setMacAddress(const QString &macAddress);
    virtual void updateScaling();

signals:
    void cycleMacAddressClick();

private slots:
    void onUpdateClick();

private:
    QString caption_;
    QString text_;
    IconButton *btnUpdate_;
    DividerLine *dividerLine_;
    bool isEditMode_;

    void updatePositions();

};

} // namespace PreferencesWindow


#endif // MACADDRESSITEM_H
