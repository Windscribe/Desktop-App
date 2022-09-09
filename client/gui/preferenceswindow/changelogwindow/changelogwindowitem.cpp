#include "changelogwindowitem.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

ChangelogWindowItem::ChangelogWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent)
{
    Q_UNUSED(preferences);
    Q_UNUSED(preferencesHelper);

    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    group_ = new PreferenceGroup(this, "", "");
    changelogItem_ = new ChangelogItem(group_);
    group_->addItem(changelogItem_);

    addItem(group_);
}

QString ChangelogWindowItem::caption() const
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Changelog");
}

} // namespace PreferencesWindow