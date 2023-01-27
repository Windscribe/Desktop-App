#ifndef MACADDRESSITEM_H
#define MACADDRESSITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class MacAddressItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit MacAddressItem(ScalableGraphicsObject *parent, const QString &caption);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setMacAddress(const QString &macAddress);
    void updateScaling() override;

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
