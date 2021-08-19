#ifndef AUTOROTATEMACITEM_H
#define AUTOROTATEMACITEM_H

#include "../baseitem.h"
#include "commongraphics/checkboxbutton.h"
#include "../dividerline.h"

#include <QFont>

namespace PreferencesWindow {

class AutoRotateMacItem : public BaseItem
{
    Q_OBJECT
public:
    explicit AutoRotateMacItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setState(bool isChecked);
    void updateScaling() override;

signals:
    void stateChanged(bool isChecked);

private:
    QString strCaption_;

    CheckBoxButton *checkBoxButton_;
    DividerLine *dividerLine;

    void updatePositions();

};

} // namespace PreferencesWindow

#endif // AUTOROTATEMACITEM_H
