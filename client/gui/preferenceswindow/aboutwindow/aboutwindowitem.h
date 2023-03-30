#ifndef ABOUTWINDOWITEM_H
#define ABOUTWINDOWITEM_H

#include "commongraphics/basepage.h"
#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"
#include "../linkitem.h"
#include "../preferencegroup.h"

namespace PreferencesWindow {

class AboutWindowItem : public CommonGraphics::BasePage
{
public:
    explicit AboutWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const;

private:
    PreferenceGroup *group_;
};

} // namespace PreferencesWindow

#endif // ABOUTWINDOWITEM_H