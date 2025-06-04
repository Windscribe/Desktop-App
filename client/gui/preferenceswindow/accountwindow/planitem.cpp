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
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), planBytes_(-1), isPremium_(false)
{
    textButton_ = new CommonGraphics::TextButton("", FontDescr(12, QFont::Normal), QColor(85, 255, 138), true, this);
    connect(textButton_, &CommonGraphics::TextButton::clicked, this, &PlanItem::upgradeClicked);
    textButton_->setClickable(false);
    textButton_->setMarginHeight(0);
    textButton_->setTextAlignment(Qt::AlignLeft);
    textButton_->setCurrentOpacity(OPACITY_HALF);
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
    QFont font = FontManager::instance().getFont(12, QFont::DemiBold);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_FULL);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE, -PREFERENCES_MARGIN_X*G_SCALE, -PREFERENCES_MARGIN_Y*G_SCALE), Qt::AlignLeft, tr("Plan Type"));

    if (isPremium_) {
        iconButton_->draw(boundingRect().width() - iconButton_->width() - PREFERENCES_MARGIN_X*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE, iconButton_->width(), iconButton_->height(), painter);
    }
}

void PlanItem::setPlan(qint64 plan)
{
    planBytes_ = plan;
    update();
}

void PlanItem::setIsPremium(bool isPremium)
{
    isPremium_ = isPremium;
    updatePositions();
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
        textButton_->setText(tr("Free"));
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
