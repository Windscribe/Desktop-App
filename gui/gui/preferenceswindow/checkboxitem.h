#ifndef CHECKBOXITEM_H
#define CHECKBOXITEM_H

#include "baseitem.h"
#include "commongraphics/checkboxbutton.h"
#include "dividerline.h"

namespace PreferencesWindow {

class CheckBoxItem : public BaseItem
{
    Q_OBJECT
public:
    explicit CheckBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setState(bool isChecked);
    bool isChecked() const;

    void setLineVisible(bool visible);
    QPointF getButtonScenePos() const;

    virtual void updateScaling();

signals:
    void stateChanged(bool isChecked);
    void buttonHoverEnter();
    void buttonHoverLeave();

private:
    QString strCaption_;
    QString strTooltip_;

    CheckBoxButton *checkBoxButton_;

    DividerLine *line_;

};

} // namespace PreferencesWindow

#endif // CHECKBOXITEM_H
