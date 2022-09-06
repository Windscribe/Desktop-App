#ifndef CHANGELOGWINDOWITEM_H
#define CHANGELOGWINDOWITEM_H

#include "commongraphics/basepage.h"
#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"
#include "preferenceswindow/preferencegroup.h"
#include "changelogitem.h"

namespace PreferencesWindow {

class ChangelogWindowItem : public CommonGraphics::BasePage
{
public:
    explicit ChangelogWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const;

private:
    PreferenceGroup *group_;
    ChangelogItem *changelogItem_;
};

} // namespace PreferencesWindow

#endif // CHANGELOGWINDOWITEM_H