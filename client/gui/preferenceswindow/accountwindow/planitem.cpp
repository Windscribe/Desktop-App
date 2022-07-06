#include "planitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

PlanItem::PlanItem(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), planBytes_(-1), isPremium_(false)
{
    generatePlanString();

    textButton_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Pro"), FontDescr(12, false), QColor(85, 255, 138), true, this);
    connect(textButton_, &CommonGraphics::TextButton::clicked, this, &PlanItem::upgradeClicked);
    textButton_->setClickable(false);
    textButton_->setTextAlignment(Qt::AlignLeft);
    updateTextButtonPos();
}

void PlanItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_FULL);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE), Qt::AlignLeft, planStr_);
}

void PlanItem::setPlan(qint64 plan)
{
    planBytes_ = plan;
    generatePlanString();
    update();
}

void PlanItem::setIsPremium(bool isPremium)
{
    isPremium_ = isPremium;
    if (isPremium_)
    {
        textButton_->setText(tr("Pro"));
        textButton_->setCurrentOpacity(OPACITY_FULL);
        textButton_->setColor(QColor(85, 255, 138));
    }
    else
    {
        textButton_->setText(tr("Upgrade"));
        textButton_->setCurrentOpacity(OPACITY_HALF);
        textButton_->setColor(Qt::white);
    }
    textButton_->setClickable(!isPremium_);
    updateTextButtonPos();
    update();
}

bool PlanItem::isPremium()
{
    return isPremium_;
}

void PlanItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updateTextButtonPos();
}

void PlanItem::onLanguageChanged()
{
    if (isPremium_)
    {
        textButton_->setText(tr("Pro"));
    }
    else
    {
        textButton_->setText(tr("Upgrade"));
    }
    updateTextButtonPos();
}

void PlanItem::generatePlanString()
{
    if (planBytes_ < 0)
    {
        planStr_ = tr("Unlimited GB");
    }
    else
    {
        planStr_ = QString(tr("%1/Month")).arg(Utils::humanReadableByteCount(planBytes_, false));
    }
}

void PlanItem::updateTextButtonPos()
{
    textButton_->setPos(boundingRect().width() - textButton_->boundingRect().width() - PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE);
}

} // namespace PreferencesWindow
