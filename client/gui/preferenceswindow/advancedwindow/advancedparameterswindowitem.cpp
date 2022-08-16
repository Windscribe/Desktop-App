#include "advancedparameterswindowitem.h"


namespace PreferencesWindow {

AdvancedParametersWindowItem::AdvancedParametersWindowItem(ScalableGraphicsObject *parent, Preferences *preferences)
  : CommonGraphics::BasePage(parent), preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences, &Preferences::debugAdvancedParametersChanged, this, &AdvancedParametersWindowItem::onAdvancedParametersPreferencesChanged);

    advancedParametersItem_ = new AdvancedParametersItem(this);
    advancedParametersItem_->setHeight(340);
    advancedParametersItem_->setAdvancedParameters(preferences->debugAdvancedParameters());

    connect(advancedParametersItem_, &AdvancedParametersItem::advancedParametersChanged, this, &AdvancedParametersWindowItem::onAdvancedParametersChanged);
    addItem(advancedParametersItem_);

}

QString AdvancedParametersWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Advanced Param...");
}

void AdvancedParametersWindowItem::setHeight(int newHeight)
{
    advancedParametersItem_->setHeight(newHeight);
}

void AdvancedParametersWindowItem::onAdvancedParametersPreferencesChanged(const QString &text)
{
    advancedParametersItem_->setAdvancedParameters(text);
}

void AdvancedParametersWindowItem::onAdvancedParametersChanged(const QString &text)
{
    preferences_->setDebugAdvancedParameters(text);
}


} // namespace PreferencesWindow
