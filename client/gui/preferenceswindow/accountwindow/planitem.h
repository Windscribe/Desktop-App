#ifndef PLANITEM_H
#define PLANITEM_H

#include "commongraphics/baseitem.h"
#include "commongraphics/textbutton.h"

namespace PreferencesWindow {

class PlanItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit PlanItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void setPlan(qint64 plan);
    void setIsPremium(bool isPremium);
    bool isPremium();

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

    void generatePlanString();
    void updateTextButtonPos();

    const char *PRO_TEXT = QT_TR_NOOP("Pro");
    const char *UPGRADE_TEXT = QT_TR_NOOP("Upgrade");
};

} // namespace PreferencesWindow

#endif // PLANITEM_H
