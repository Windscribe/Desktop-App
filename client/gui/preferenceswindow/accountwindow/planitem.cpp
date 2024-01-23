#include "planitem.h"

#include <QLocale>
#include <QPainter>

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

PlanItem::PlanItem(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), planBytes_(-1), isPremium_(false)
{
    generatePlanString();

    textButton_ = new CommonGraphics::TextButton(tr(PRO_TEXT), FontDescr(12, false), QColor(85, 255, 138), true, this);
    connect(textButton_, &CommonGraphics::TextButton::clicked, this, &PlanItem::upgradeClicked);
    textButton_->setClickable(false);
    textButton_->setMarginHeight(0);
    textButton_->setTextAlignment(Qt::AlignLeft);
    updateTextButtonPos();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &PlanItem::onLanguageChanged);
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
        textButton_->setText(tr(PRO_TEXT));
        textButton_->setCurrentOpacity(OPACITY_FULL);
        textButton_->setColor(QColor(85, 255, 138));
    }
    else
    {
        textButton_->setText(tr(UPGRADE_TEXT));
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
    if (isPremium_) {
        textButton_->setText(tr(PRO_TEXT));
    } else {
        textButton_->setText(tr(UPGRADE_TEXT));
    }
    updateTextButtonPos();

    generatePlanString();
    update();
}

void PlanItem::generatePlanString()
{
    if (planBytes_ < 0) {
        planStr_ = tr("Unlimited Data");
    } else {
        QLocale locale(LanguageController::instance().getLanguage());
        planStr_ = QString(tr("%1/Month")).arg(locale.formattedDataSize(planBytes_, 0, QLocale::DataSizeTraditionalFormat));
    }
}

void PlanItem::updateTextButtonPos()
{
    textButton_->setPos(boundingRect().width() - textButton_->boundingRect().width() - PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE);
}

} // namespace PreferencesWindow
