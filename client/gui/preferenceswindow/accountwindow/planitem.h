#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/textbutton.h"
#include "graphicresources/independentpixmap.h"

namespace PreferencesWindow {

class PlanItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit PlanItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

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
    QSharedPointer<IndependentPixmap> iconButton_;
    qint64 planBytes_;
    bool isPremium_;

    void updatePositions();
};

} // namespace PreferencesWindow
