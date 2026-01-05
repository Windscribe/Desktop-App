#include "planitem.h"

#include <QLocale>
#include <QPainter>

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

PlanItem::PlanItem(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), planBytes_(-1), isPremium_(false), alcCount_(0)
{
    textButton_ = new CommonGraphics::TextButton("", FontDescr(14, QFont::Normal), FontManager::instance().getSeaGreenColor(), true, this);
    connect(textButton_, &CommonGraphics::TextButton::clicked, this, &PlanItem::upgradeClicked);
    textButton_->setClickable(false);
    textButton_->setMarginHeight(0);
    textButton_->setTextAlignment(Qt::AlignLeft);
    textButton_->setCurrentOpacity(OPACITY_SIXTY);
    textButton_->setColor(Qt::white);

    iconButton_ = ImageResourcesSvg::instance().getIndependentPixmap("PRO");
    updatePositions();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &PlanItem::onLanguageChanged);
}

void PlanItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Title
    QFont font = FontManager::instance().getFont(14, QFont::DemiBold);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_FULL);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, 0), Qt::AlignLeft | Qt::AlignVCenter, tr("Plan Type"));

    if (isPremium_) {
        iconButton_->draw(boundingRect().width() - iconButton_->width() - PREFERENCES_MARGIN_X*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE, iconButton_->width(), iconButton_->height(), painter);
    }
}

void PlanItem::setPlan(qint64 plan)
{
    planBytes_ = plan;
    onLanguageChanged(); // String may change based on plan
    update();
}

void PlanItem::setIsPremium(bool isPremium)
{
    isPremium_ = isPremium;
    onLanguageChanged(); // String may change based on plan
    updatePositions();
}

void PlanItem::setAlcCount(qsizetype count)
{
    alcCount_ = count;
    onLanguageChanged(); // String may change based on plan
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
    updatePositions();
}

void PlanItem::onLanguageChanged()
{
    if (!isPremium_) {
        if (planBytes_ == -1) {
            textButton_->setText(tr("Unlimited Data")); // Build a Plan
        } else if (alcCount_ > 0) {
            textButton_->setText(tr("Custom")); // Build a Plan, non-unlimited
        } else {
            textButton_->setText(tr("Free"));
        }
    }
    updatePositions();
}

void PlanItem::updatePositions()
{
    if (isPremium_) {
        textButton_->hide();
    } else {
        textButton_->setPos(boundingRect().width() - textButton_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE);
        textButton_->show();
    }
    update();
}

} // namespace PreferencesWindow
