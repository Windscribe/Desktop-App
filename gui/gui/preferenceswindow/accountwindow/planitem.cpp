#include "planitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

PlanItem::PlanItem(ScalableGraphicsObject *parent) : BaseItem(parent, 78),
    planBytes_(-1)
{
    generatePlanString();

    dividerLine_ = new DividerLine(this, 276);
    dividerLine_->setPos(24, 78 - 3);

    textButton_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Pro"), FontDescr(12, true), QColor(255, 255, 255), true, this);
    connect(textButton_, SIGNAL(clicked()), SIGNAL(upgradeClicked()));
    textButton_->setClickable(false);
    updateTextButtonPos();
}

void PlanItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->setOpacity(0.5 * initialOpacity);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 18*G_SCALE, 0, 0), Qt::AlignTop, tr("PLAN"));

    painter->setOpacity(1.0 * initialOpacity);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, -18*G_SCALE), Qt::AlignBottom, planStr_);
}

void PlanItem::setPlan(qint64 plan)
{
    planBytes_ = plan;
    generatePlanString();
    if (planBytes_ < 0)
    {
        textButton_->setText(tr("Pro"));
        textButton_->setCurrentOpacity(OPACITY_FULL);
        textButton_->setClickable(false);
    }
    else
    {
        textButton_->setText(tr("Get more data"));
        textButton_->setCurrentOpacity(OPACITY_UNHOVER_TEXT);
        textButton_->setClickable(true);
    }
    updateTextButtonPos();
    update();
}

void PlanItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(78*G_SCALE);
    dividerLine_->setPos(24*G_SCALE, (78 - 3)*G_SCALE);
    updateTextButtonPos();
}

void PlanItem::onLanguageChanged()
{
    if (planBytes_ < 0)
    {
        textButton_->setText(tr("Pro"));
    }
    else
    {
        textButton_->setText(tr("Get more data"));
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
        planStr_ = QString(tr("%1/Mo")).arg(Utils::humanReadableByteCount(planBytes_, false));
    }
}

void PlanItem::updateTextButtonPos()
{
    textButton_->recalcBoundingRect();

    QRectF rcBottom = boundingRect().adjusted(16*G_SCALE, 0, -16*G_SCALE, -18*G_SCALE);
    textButton_->setPos(boundingRect().width() - textButton_->boundingRect().width() - 16*G_SCALE, rcBottom.bottom() - textButton_->boundingRect().height() + 2*G_SCALE);
}


} // namespace PreferencesWindow
