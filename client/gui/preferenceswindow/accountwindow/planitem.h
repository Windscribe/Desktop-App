#ifndef PLANITEM_H
#define PLANITEM_H

#include "../baseitem.h"
#include "commongraphics/textbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class PlanItem : public BaseItem
{
    Q_OBJECT
public:
    explicit PlanItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void setPlan(qint64 plan);
    void setIsPremium(bool isPremium);

    void updateScaling() override;

signals:
    void upgradeClicked();

private slots:
    void onLanguageChanged();

private:
    CommonGraphics::TextButton *textButton_;
    qint64 planBytes_;
    bool isPremium_;
    QString planStr_;
    DividerLine *dividerLine_;

    void generatePlanString();
    void updateTextButtonPos();

};

} // namespace PreferencesWindow

#endif // PLANITEM_H
