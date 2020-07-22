#ifndef AUTOMANUALSWITCHITEM_H
#define AUTOMANUALSWITCHITEM_H

#include "../baseitem.h"
#include "CommonGraphics/textbutton.h"
#include <QFont>
#include <QVariantAnimation>

namespace PreferencesWindow {

class AutoManualSwitchItem : public BaseItem
{
    Q_OBJECT
public:
    enum SWITCH_STATE { AUTO, MANUAL};


    explicit AutoManualSwitchItem(ScalableGraphicsObject *parent, const QString &caption);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setState(SWITCH_STATE state);
    virtual void updateScaling();
signals:
    void stateChanged(AutoManualSwitchItem::SWITCH_STATE state);

private slots:
    void onClickAuto();
    void onClickManual();
    void onAnimationValueChanged(const QVariant &value);


private:
    QString strCaption_;

    CommonGraphics::TextButton *btnAuto_;
    CommonGraphics::TextButton *btnManual_;

    QVariantAnimation anim_;
    bool isExpanded_;

    qreal linePos_;     // 0.0 - auto (left) position, 1.0 manual (right) position

    void drawSelectionLine(QPainter *painter, qreal v);
    void updatePositions();

};

} // namespace PreferencesWindow

#endif // AUTOMANUALSWITCHITEM_H
